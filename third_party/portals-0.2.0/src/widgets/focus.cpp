// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "action_state.hpp"

#include <algorithm>
#include <tuple>

namespace portals {
EventRouter::EventRouter(std::unique_ptr<State> state) : state_(std::move(state)) {
}

std::expected<std::unique_ptr<EventRouter>, TreeError> EventRouter::create(WidgetTree &tree,
                                                                           EventRouterOptions options) {
  auto root = tree.root();

  if (!root) {
    return std::unexpected(root.error());
  }

  if (options.max_registrations == 0) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    auto state = std::make_unique<State>(tree, options);

    return std::unique_ptr<EventRouter>(new EventRouter(std::move(state)));
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

EventRouter::~EventRouter() {
  if (state_->owner != std::this_thread::get_id() || state_->dispatching || state_->collecting || state_->closing) {
    std::terminate();
  }

  state_->closing = true;
  state_->registrations.clear();
}

WidgetTree &EventRouter::tree() {
  return state_->tree;
}

std::expected<void, TreeError> EventRouter::check() const {
  if (state_->owner != std::this_thread::get_id()) {
    return std::unexpected(TreeError::WRONG_THREAD);
  }

  if (state_->closing) {
    return std::unexpected(TreeError::CLOSED);
  }

  if (state_->collecting) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  auto root = state_->tree.root();

  if (!root) {
    return std::unexpected(root.error());
  }

  return state_->tree.edit(*root, [](Widget &) {
  });
}

std::shared_ptr<detail::ActionRegistration> EventRouter::registration(Node<Widget> node) const {
  for (const auto &entry : state_->registrations) {
    if (entry->node == node && entry->cancellation->connected.load()) {
      return entry;
    }
  }

  return {};
}

std::expected<void, TreeError> EventRouter::collect() {
  auto allowed = check();

  if (!allowed) {
    return allowed;
  }

  for (auto &entry : state_->registrations) {
    auto live = tree().inputEligible(entry->node);

    if (!live && live.error() != TreeError::STALE_NODE) {
      return std::unexpected(live.error());
    }

    if (!live) {
      entry->cancellation->connected.store(false);
    }
  }

  // Destructors of captures cannot reenter while registry storage is being changed.
  struct CollectionGuard {
    bool &active;

    explicit CollectionGuard(bool &value) : active(value) {
      active = true;
    }

    ~CollectionGuard() {
      active = false;
    }
  } guard(state_->collecting);

  // Active delivery keeps its own shared owner until the callback returns.
  std::erase_if(state_->registrations, [](const auto &entry) {
    return !entry->cancellation->connected.load();
  });
  std::erase_if(state_->history, [&](auto node) {
    return !registration(node);
  });

  return {};
}

std::expected<std::vector<Node<Widget>>, TreeError> EventRouter::focusOrder() {
  std::vector<Node<Widget>> result(0);
  std::vector<Node<Widget>> pending(0);
  auto root = tree().root();

  if (!root) {
    return std::unexpected(root.error());
  }

  // Inspection does not call user code or mutate registrations on this UI thread.
  // Borrow entries only for this walk; keep cancellation checks live for worker tokens.
  std::vector<const detail::ActionRegistration *> index(0);
  const auto identity = [](const detail::ActionRegistration *entry) {
    const auto id = entry->node.id();

    return std::tuple{id.session, id.slot, id.generation};
  };

  index.reserve(state_->registrations.size());

  for (const auto &entry : state_->registrations) {
    if (entry->focusable) {
      index.push_back(entry.get());
    }
  }

  std::ranges::sort(index, {}, identity);
  pending.push_back(*root);

  while (!pending.empty()) {
    const auto node = pending.back();
    auto snapshot = tree().inspect(node);

    pending.pop_back();

    if (!snapshot) {
      return std::unexpected(snapshot.error());
    }

    if (!snapshot->visible || !snapshot->enabled) {
      continue;
    }

    const auto id = node.id();
    const auto found = std::ranges::lower_bound(index, std::tuple{id.session, id.slot, id.generation}, {}, identity);

    if (found != index.end() && (*found)->node == node && (*found)->cancellation->connected.load()) {
      result.push_back(node);
    }

    // Reverse-push onto the LIFO stack visits siblings in insertion order. Skipping an
    // ineligible ancestor prunes the whole subtree without consulting arranged geometry.
    pending.insert(pending.end(), snapshot->children.rbegin(), snapshot->children.rend());
  }

  // Copy inside the allocation boundary; MSVC debug vector moves may allocate in noexcept.
  return std::expected<std::vector<Node<Widget>>, TreeError>(std::in_place, result);
}

std::expected<void, TreeError> EventRouter::changeFocus(Node<Widget> node) {
  if (node == state_->focus) {
    return {};
  }

  // Allocate history before publishing the new focus.
  if (node.id().session != 0) {
    auto found = std::ranges::find(state_->history, node);

    if (found != state_->history.end()) {
      state_->history.erase(found);
    }

    state_->history.push_back(node);
  }

  const auto previous = state_->focus;

  state_->focus = node;

  if (auto *recorder = tree().diagnostics(); recorder && recorder->enabled()) {
    const auto id = node.id();

    recorder->note(DiagnosticEvent::FOCUS, DiagnosticReason::NONE, {id.session, id.slot, id.generation});
  }

  if (previous.id().session != 0) {
    auto status = tree().invalidate(previous, Invalidation::PAINT);

    if (!status && status.error() != TreeError::STALE_NODE) {
      return status;
    }
  }

  if (node.id().session != 0) {
    return tree().invalidate(node, Invalidation::PAINT);
  }

  return {};
}

std::expected<Node<Widget>, TreeError> EventRouter::focused() {
  auto allowed = collect();

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  try {
    auto order = focusOrder();

    if (!order) {
      return std::unexpected(order.error());
    }

    if (std::ranges::contains(*order, state_->focus)) {
      return state_->focus;
    }

    Node<Widget> next;

    // Recover from still-eligible focus history before choosing a never-focused preorder node.
    // Hiding a subtree need not erase its history, so restoring it can make that entry useful again.
    for (auto it = state_->history.rbegin(); it != state_->history.rend(); ++it) {
      if (std::ranges::contains(*order, *it)) {
        next = *it;

        break;
      }
    }

    if (next.id().session == 0 && !order->empty()) {
      next = order->front();
    }

    auto changed = changeFocus(next);

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return next;
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<void, TreeError> EventRouter::setFocus(Node<Widget> node) {
  auto allowed = collect();

  if (!allowed) {
    return allowed;
  }

  auto eligible = tree().inputEligible(node);

  if (!eligible) {
    return std::unexpected(eligible.error());
  }

  auto entry = registration(node);

  if (!*eligible || !entry || !entry->focusable) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    return changeFocus(node);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<Node<Widget>, TreeError> EventRouter::traverse(bool reverse) {
  auto allowed = collect();

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  try {
    auto order = focusOrder();

    if (!order) {
      return std::unexpected(order.error());
    }

    Node<Widget> next;

    if (!order->empty()) {
      auto position = std::ranges::find(*order, state_->focus);

      if (position == order->end()) {
        next = reverse ? order->back() : order->front();
      } else if (reverse) {
        next = position == order->begin() ? order->back() : *std::prev(position);
      } else {
        ++position;
        next = position == order->end() ? order->front() : *position;
      }
    }

    auto changed = changeFocus(next);

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return next;
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<Node<Widget>, TreeError> EventRouter::hitTest(CellPoint point) {
  auto allowed = check();

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  try {
    auto root = tree().root();
    std::vector<Node<Widget>> pending{*root};
    Node<Widget> hit;

    while (!pending.empty()) {
      const auto node = pending.back();
      auto snapshot = tree().inspect(node);

      pending.pop_back();

      if (!snapshot) {
        return std::unexpected(snapshot.error());
      }

      const auto &geometry = snapshot->geometry;

      if (!snapshot->visible || !snapshot->enabled || !geometry.valid || !geometry.surface.contains(point) ||
          !geometry.clip.contains(point)) {
        continue;
      }

      // Retaining the last preorder hit gives descendants precedence over their ancestors
      // and later sibling subtrees precedence over earlier ones, matching painting order.
      hit = node;
      pending.insert(pending.end(), snapshot->children.rbegin(), snapshot->children.rend());
    }

    return hit;
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}
} // namespace portals
