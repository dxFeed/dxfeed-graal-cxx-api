// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "action_state.hpp"

#include <optional>

namespace portals {
namespace {
std::optional<Action> keyAction(InputKey key) {
  switch (key) {
  case InputKey::ENTER:
    return Action::ACTIVATE;
  case InputKey::ESCAPE:
    return Action::CANCEL;
  case InputKey::LEFT:
    return Action::MOVE_LEFT;
  case InputKey::RIGHT:
    return Action::MOVE_RIGHT;
  case InputKey::UP:
    return Action::MOVE_UP;
  case InputKey::DOWN:
    return Action::MOVE_DOWN;
  case InputKey::HOME:
    return Action::MOVE_HOME;
  case InputKey::END:
    return Action::MOVE_END;
  case InputKey::BACKSPACE:
    return Action::ERASE_BACKWARD;
  case InputKey::DELETE_KEY:
    return Action::ERASE_FORWARD;
  default:
    return std::nullopt;
  }
}

DiagnosticNodeId diagnosticNode(NodeId id) {
  return {id.session, id.slot, id.generation};
}

struct RoutingTrace {
  Diagnostics *recorder;
  DiagnosticScope scope;

  RoutingTrace(Diagnostics *value, DiagnosticInput input)
      : recorder(value), scope(value, value && value->enabled()
                                        ? (value->current().correlationId ? value->current() : value->beginInput(input))
                                        : DiagnosticContext{}) {
  }

  ~RoutingTrace() {
    if (recorder && recorder->enabled()) {
      if (!scope.frameRequested()) {
        recorder->note(DiagnosticEvent::FRAME_NOT_REQUESTED, DiagnosticReason::FRAME_NOT_REQUESTED);
      }

      recorder->note(DiagnosticEvent::COMPLETED, scope.outcome());
    }
  }
};

struct DeliveryGuard {
  bool &active;

  explicit DeliveryGuard(bool &value) : active(value) {
    active = true;
  }

  ~DeliveryGuard() {
    active = false;
  }
};
} // namespace

ActionBinding::ActionBinding(std::weak_ptr<detail::ActionCancellation> registration)
    : registration_(std::move(registration)) {
}

ActionBinding::~ActionBinding() {
  disconnect();
}

ActionBinding::ActionBinding(ActionBinding &&other) noexcept : registration_(std::move(other.registration_)) {
}

ActionBinding &ActionBinding::operator=(ActionBinding &&other) noexcept {
  if (this != &other) {
    disconnect();
    registration_ = std::move(other.registration_);
  }

  return *this;
}

void ActionBinding::disconnect() noexcept {
  if (auto entry = registration_.lock()) {
    entry->connected.store(false);
  }

  registration_.reset();
}

std::expected<void, TreeError> EventRouter::canBind(Node<Widget> node) {
  auto allowed = check();

  if (!allowed) {
    return allowed;
  }

  if (state_->dispatching) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  auto live = tree().inputEligible(node);

  if (!live) {
    return std::unexpected(live.error());
  }

  auto collected = collect();

  if (!collected) {
    return collected;
  }

  // Collecting captures can mutate the tree through their destructors.
  live = tree().inputEligible(node);

  if (!live) {
    return std::unexpected(live.error());
  }

  if (registration(node)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  if (state_->registrations.size() >= state_->options.max_registrations) {
    return std::unexpected(TreeError::NODE_LIMIT);
  }

  return {};
}

std::expected<ActionBinding, TreeError>
EventRouter::add(Node<Widget> node, std::function<std::expected<EventReply, TreeError>(const RoutedEvent &)> callback,
                 bool focusable) {
  auto entry = std::make_shared<detail::ActionRegistration>();

  entry->cancellation = std::make_shared<detail::ActionCancellation>();
  entry->node = node;
  entry->focusable = focusable;
  entry->callback = std::move(callback);

  // Callback construction/move may reenter, remove the target, or fill the registry.
  auto allowed = canBind(node);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  state_->registrations.push_back(entry);

  return ActionBinding(entry->cancellation);
}

std::expected<DispatchResult, TreeError> EventRouter::route(Node<Widget> target, const RoutedEvent &event) {
  DispatchResult result;
  auto *recorder = tree().diagnostics();
  const auto trace = [&](DiagnosticEvent stage, DiagnosticReason reason) {
    if (recorder && recorder->enabled()) {
      recorder->note(stage, reason, diagnosticNode(target.id()));
    }
  };

  if (const auto *action = std::get_if<ActionEvent>(&event); recorder && recorder->enabled() && action) {
    recorder->record({.context = recorder->current(),
                      .event = DiagnosticEvent::ACTION,
                      .semanticAction = static_cast<std::uint32_t>(action->action) + 1});
  }

  if (target.id().session == 0) {
    trace(DiagnosticEvent::TARGET, DiagnosticReason::NO_TARGET);

    return result;
  }

  auto eligible = tree().inputEligible(target);

  if (!eligible) {
    trace(DiagnosticEvent::TARGET, DiagnosticReason::STALE_TARGET);

    return std::unexpected(eligible.error());
  }

  if (!*eligible) {
    trace(DiagnosticEvent::TARGET, DiagnosticReason::TARGET_DISABLED);

    return result;
  }

  trace(DiagnosticEvent::TARGET, DiagnosticReason::NONE);

  if (target == state_->focus) {
    trace(DiagnosticEvent::FOCUS, DiagnosticReason::NONE);
  }

  result.target = target.id();
  result.status = DispatchStatus::UNHANDLED;

  std::vector<Node<Widget>> path(0);
  auto current = target;

  while (current.id().session != 0) {
    path.push_back(current);

    auto snapshot = tree().inspect(current);

    if (!snapshot) {
      return std::unexpected(snapshot.error());
    }

    current = snapshot->parent;
  }

  // Snapshot the ancestry before any handler can mutate it. Revalidate each admission so
  // removal/hiding/disabling suppresses that node while surviving original ancestors can run.
  // The local registration owner keeps captures alive through self-disconnect and collection.
  bool hadHandler{};

  for (auto node : path) {
    auto live = tree().inputEligible(node);

    if (!live) {
      if (live.error() == TreeError::STALE_NODE || live.error() == TreeError::CLOSED) {
        continue;
      }

      return std::unexpected(live.error());
    }

    if (!*live) {
      continue;
    }

    auto entry = registration(node);

    if (!entry) {
      continue;
    }

    hadHandler = true;

    DiagnosticScope handlerScope(recorder, recorder && recorder->enabled() ? recorder->current() : DiagnosticContext{});

    if (recorder && recorder->enabled()) {
      recorder->note(DiagnosticEvent::HANDLER_STARTED, DiagnosticReason::NONE, diagnosticNode(node.id()));
    }

    auto reply = entry->callback(event);

    if (recorder && recorder->enabled()) {
      recorder->note(DiagnosticEvent::HANDLER_RETURNED,
                     reply ? handlerScope.outcome() : DiagnosticReason::HANDLER_FAILED, diagnosticNode(node.id()));
    }

    if (!reply) {
      return std::unexpected(reply.error());
    }

    if (*reply == EventReply::HANDLED) {
      result.handler = node.id();
      result.status = DispatchStatus::HANDLED;

      break;
    }
  }

  if (result.status == DispatchStatus::UNHANDLED) {
    trace(DiagnosticEvent::ACTION, hadHandler ? DiagnosticReason::UNHANDLED : DiagnosticReason::UNBOUND_ACTION);
  }

  return result;
}

std::expected<DispatchResult, TreeError> EventRouter::invoke(Node<Widget> target, ActionEvent action) {
  auto allowed = collect();

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  if (state_->dispatching) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  DeliveryGuard guard(state_->dispatching);
  RoutingTrace diagnostic(tree().diagnostics(), DiagnosticInput::OTHER);

  try {
    return route(target, RoutedEvent(action));
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<DispatchResult, TreeError> EventRouter::dispatch(const InputEvent &event) {
  auto allowed = collect();

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  if (state_->dispatching) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  DeliveryGuard guard(state_->dispatching);
  auto *recorder = tree().diagnostics();
  DiagnosticInput inputKind{DiagnosticInput::NONE};

  if (recorder && recorder->enabled()) {
    inputKind = std::holds_alternative<KeyEvent>(event)          ? DiagnosticInput::KEY
                : std::holds_alternative<TextInputEvent>(event)  ? DiagnosticInput::TEXT
                : std::holds_alternative<PasteInputEvent>(event) ? DiagnosticInput::PASTE
                : std::holds_alternative<MouseInputEvent>(event) ? DiagnosticInput::MOUSE
                                                                 : DiagnosticInput::OTHER;
  }

  RoutingTrace diagnostic(recorder, inputKind);

  try {
    if (const auto *key = std::get_if<KeyEvent>(&event)) {
      if (key->key == InputKey::TAB && key->action != KeyAction::RELEASE && !key->modifiers.alt &&
          !key->modifiers.control && !key->modifiers.super) {
        auto next = traverse(key->modifiers.shift);

        if (!next) {
          return std::unexpected(next.error());
        }

        if (recorder && recorder->enabled()) {
          recorder->note(DiagnosticEvent::TARGET,
                         next->id().session ? DiagnosticReason::NONE : DiagnosticReason::NO_TARGET,
                         diagnosticNode(next->id()));
          recorder->note(DiagnosticEvent::ACTION);
        }

        return DispatchResult{
          next->id(), {}, next->id().session == 0 ? DispatchStatus::NO_TARGET : DispatchStatus::HANDLED};
      }

      auto target = focused();

      if (!target) {
        return std::unexpected(target.error());
      }

      auto action = keyAction(key->key);

      if (action && key->action != KeyAction::RELEASE) {
        return route(*target, ActionEvent{*action, key->modifiers});
      }

      return route(*target, *key);
    }

    // Mouse targeting uses geometry independently of focus. A left press updates focus on
    // the nearest focusable ancestor, but bubbles the original mouse event from the actual hit.
    if (const auto *mouse = std::get_if<MouseInputEvent>(&event)) {
      auto target = hitTest({mouse->column, mouse->row});

      if (!target) {
        return std::unexpected(target.error());
      }

      if (mouse->button == MouseButton::LEFT && mouse->action == MouseAction::PRESS) {
        auto candidate = *target;

        while (candidate.id().session != 0) {
          auto entry = registration(candidate);

          if (entry && entry->focusable) {
            auto changed = changeFocus(candidate);

            if (!changed) {
              return std::unexpected(changed.error());
            }

            break;
          }

          auto snapshot = tree().inspect(candidate);

          if (!snapshot) {
            return std::unexpected(snapshot.error());
          }

          candidate = snapshot->parent;
        }
      }

      return route(*target, *mouse);
    }

    if (const auto *text = std::get_if<TextInputEvent>(&event)) {
      auto target = focused();

      if (!target) {
        return std::unexpected(target.error());
      }

      return route(*target, *text);
    }

    if (const auto *paste = std::get_if<PasteInputEvent>(&event)) {
      auto target = focused();

      if (!target) {
        return std::unexpected(target.error());
      }

      return route(*target, *paste);
    }

    if (recorder && recorder->enabled()) {
      recorder->note(DiagnosticEvent::TARGET, DiagnosticReason::NO_TARGET);
    }

    return DispatchResult{};
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}
} // namespace portals
