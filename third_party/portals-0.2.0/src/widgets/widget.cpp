// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/widget.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <thread>

namespace portals {
namespace {
constexpr std::size_t MAXIMUM_TREE_DEPTH = 256;

std::atomic<std::uint64_t> next_session{1};

std::uint64_t issueSession() noexcept {
  auto value = next_session.load(std::memory_order_relaxed);

  while (value != std::numeric_limits<std::uint64_t>::max()) {
    if (next_session.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
      return value;
    }
  }

  return 0;
}

constexpr std::uint64_t nextGeneration(std::uint64_t value) noexcept {
  return value == std::numeric_limits<std::uint64_t>::max() ? 0 : value + 1;
}

static_assert(nextGeneration(std::numeric_limits<std::uint64_t>::max()) == 0);
static_assert(nextGeneration(std::numeric_limits<std::uint64_t>::max() - 1) ==
              std::numeric_limits<std::uint64_t>::max());
} // namespace

struct WidgetTree::State {
  struct Record {
    NodeId id;

    Record *parent{};
    // The count constructor permits allocation failures from debug iterator proxies.
    std::vector<std::unique_ptr<Record>> children = std::vector<std::unique_ptr<Record>>(0);
    std::unique_ptr<Widget> widget;
    UiScope scope;
    NodeGeometry geometry;
    std::size_t depth{};

    bool live{true};
    bool visible{true};
    bool enabled{true};
  };

  struct Slot {
    Record *record{};

    std::uint64_t generation{1};
  };

  Scheduler &scheduler;

  WidgetTreeOptions options;
  ErrorSink sink;

  void *context;

  const std::thread::id owner{std::this_thread::get_id()};
  std::uint64_t session{};
  std::vector<Slot> slots;
  std::unique_ptr<Record> root;
  std::size_t count{};
  std::size_t borrows{};
  std::size_t frames{};
  DiagnosticContext pending_diagnostic{};
  DiagnosticContext frame_diagnostic{};
  std::size_t lifecycle{};

  bool extension{};
  bool reclaiming{};
  bool removal_pending{};
  bool closed{};
  bool reporting{};

  Invalidation dirty{Invalidation::LAYOUT};

  State(Scheduler &value, WidgetTreeOptions limits, ErrorSink error_sink, void *error_context)
      : scheduler(value), options(limits), sink(error_sink), context(error_context), slots(limits.max_nodes) {
  }

  std::expected<void, TreeError> check() const {
    if (std::this_thread::get_id() != owner) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    if (closed) {
      return std::unexpected(TreeError::CLOSED);
    }

    return {};
  }

  std::expected<Record *, TreeError> find(NodeId id) const {
    if (auto result = check(); !result) {
      return std::unexpected(result.error());
    }

    if (id.session == 0) {
      return std::unexpected(TreeError::STALE_NODE);
    }

    if (id.session != session) {
      return std::unexpected(TreeError::WRONG_SESSION);
    }

    if (id.slot >= slots.size()) {
      return std::unexpected(TreeError::STALE_NODE);
    }

    const auto &slot = slots[id.slot];

    if (!slot.record || !slot.record->live || slot.generation != id.generation) {
      return std::unexpected(TreeError::STALE_NODE);
    }

    return slot.record;
  }

  static void invalidateGeometry(Record &record) noexcept {
    record.geometry.valid = false;

    for (auto &child : record.children) {
      if (child) {
        invalidateGeometry(*child);
      }
    }
  }

  void invalidate(Invalidation kind) noexcept {
    if (auto *recorder = scheduler.diagnostics(); recorder && recorder->enabled()) {
      auto traceContext = recorder->current();

      if (kind != Invalidation::NONE && frames == 0) {
        if (!pending_diagnostic.frameId || pending_diagnostic.period != traceContext.period) {
          pending_diagnostic = traceContext;
          pending_diagnostic.frameId = recorder->nextIdentity();
        }

        traceContext.frameId = pending_diagnostic.frameId;
        recorder->record({.context = traceContext, .event = DiagnosticEvent::FRAME_REQUESTED});
      }

      recorder->record({.context = traceContext,
                        .event = DiagnosticEvent::INVALIDATION,
                        .reason = kind == Invalidation::NONE ? DiagnosticReason::FRAME_NOT_REQUESTED
                                                             : DiagnosticReason::VALUE_CHANGED});
    }

    if (kind == Invalidation::LAYOUT) {
      dirty = kind;

      if (root) {
        invalidateGeometry(*root);
      }
    } else if (kind == Invalidation::PAINT && dirty == Invalidation::NONE) {
      dirty = kind;
    }
  }

  static void markDead(Record &record) noexcept {
    record.live = false;
    record.geometry.valid = false;

    for (auto &child : record.children) {
      if (child) {
        markDead(*child);
      }
    }
  }

  static void closeScopes(Record &record) {
    for (auto &child : record.children) {
      if (child) {
        closeScopes(*child);
      }
    }

    (void)record.scope.close();
  }

  static bool shown(const Record &record, bool input) noexcept {
    for (auto current = &record; current; current = current->parent) {
      if (!current->visible || (input && !current->enabled)) {
        return false;
      }
    }

    return true;
  }
};

void Widget::mount() {
}

void Widget::unmount() {
}

ComputedStyle Widget::styleDefaults() const {
  return {};
}

const std::optional<ComputedStyle> &Widget::computedStyle() const noexcept {
  return computedStyle_;
}

std::uint64_t Widget::styleRevision() const noexcept {
  return styleRevision_;
}

std::expected<CellSize, TreeError> Widget::measure(CellSize) {
  return CellSize{};
}

std::expected<void, TreeError> Widget::arrange(CellRect) {
  return {};
}

std::expected<void, TreeError> Widget::paint(PaintContext &) {
  return {};
}

WidgetTree::WidgetTree(std::unique_ptr<State> state) : state_(std::move(state)) {
}

WidgetTree::Borrow::Borrow(WidgetTree &value, bool is_frame) : tree(value), frame(is_frame) {
  ++tree.state_->borrows;

  if (frame) {
    ++tree.state_->frames;
  }
}

WidgetTree::Borrow::~Borrow() {
  if (frame) {
    --tree.state_->frames;
  }

  // Removal revokes identities immediately, but nested callbacks can still hold references.
  // The final borrow is the only safe point at which unmount/destruction may reclaim storage.
  if (--tree.state_->borrows == 0) {
    tree.reclaim();
  }
}

TreeError WidgetTree::fromScheduler(SchedulerError error) {
  switch (error) {
  case SchedulerError::INVALID_ARGUMENT:
    return TreeError::INVALID_ARGUMENT;
  case SchedulerError::WRONG_THREAD:
    return TreeError::WRONG_THREAD;
  case SchedulerError::WRONG_SCHEDULER:
    return TreeError::WRONG_SESSION;
  case SchedulerError::CLOSED:
    return TreeError::CLOSED;
  case SchedulerError::WOULD_BLOCK:
  case SchedulerError::REENTRANT_DISPATCH:
    return TreeError::WOULD_BLOCK;
  case SchedulerError::NO_MEMORY:
    return TreeError::NO_MEMORY;
  case SchedulerError::IDENTITY_EXHAUSTED:
    return TreeError::IDENTITY_EXHAUSTED;
  case SchedulerError::CALLBACK_FAILED:
    return TreeError::CALLBACK_FAILED;
  }

  return TreeError::INVALID_ARGUMENT;
}

std::expected<std::unique_ptr<WidgetTree>, TreeError>
WidgetTree::create(Scheduler &scheduler, WidgetTreeOptions options, ErrorSink sink, void *context) {
  auto scope = scheduler.makeScope();

  if (!scope) {
    return std::unexpected(fromScheduler(scope.error()));
  }

  if (options.max_nodes == 0 || options.max_depth == 0 || options.max_depth > MAXIMUM_TREE_DEPTH ||
      options.max_nodes > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(State::Slot)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    auto state = std::make_unique<State>(scheduler, options, sink, context);

    state->session = issueSession();

    if (!state->session) {
      return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
    }

    state->root = std::make_unique<State::Record>();

    auto &root = *state->root;

    root.widget = std::make_unique<Widget>();
    root.scope = std::move(*scope);
    root.id = {state->session, 0, 1};
    root.depth = 1;

    state->slots[0].record = &root;
    state->count = 1;

    auto tree = std::unique_ptr<WidgetTree>(new WidgetTree(std::move(state)));

    root.widget->mount();

    return tree;
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

WidgetTree::~WidgetTree() {
  if (std::this_thread::get_id() != state_->owner || state_->borrows || state_->reclaiming) {
    std::terminate();
  }

  (void)close();
}

void WidgetTree::report(NodeId id, TreeError error) noexcept {
  if (state_->sink && !state_->reporting) {
    state_->reporting = true;
    state_->sink(state_->context, id, error);
    state_->reporting = false;
  }
}

std::expected<Widget *, TreeError> WidgetTree::access(NodeId id, bool mutation) const {
  auto value = state_->find(id);

  if (!value) {
    return std::unexpected(value.error());
  }

  if (mutation && state_->frames) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  return (*value)->widget.get();
}

std::expected<Node<Widget>, TreeError> WidgetTree::root() const {
  if (auto result = state_->check(); !result) {
    return std::unexpected(result.error());
  }

  if (!state_->root || !state_->root->live) {
    return std::unexpected(TreeError::STALE_NODE);
  }

  return Node<Widget>(state_->root->id);
}

std::expected<NodeSnapshot, TreeError> WidgetTree::inspect(Node<Widget> node) const {
  auto value = state_->find(node.id());

  if (!value) {
    return std::unexpected(value.error());
  }

  const auto &record = **value;

  try {
    NodeSnapshot result;

    if (record.parent) {
      result.parent = Node<Widget>(record.parent->id);
    }

    result.geometry = record.geometry;
    result.visible = record.visible;
    result.enabled = record.enabled;
    result.children.reserve(record.children.size());

    for (const auto &child : record.children) {
      if (child && child->live) {
        result.children.push_back(Node<Widget>(child->id));
      }
    }

    // Avoid a noexcept vector move allocating a debug iterator proxy on MSVC.
    // The copy may throw inside this boundary; the expected prvalue is elided.
    return std::expected<NodeSnapshot, TreeError>(std::in_place, result);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<void, TreeError> WidgetTree::canInsert(NodeId parent) const {
  auto record = state_->find(parent);

  if (!record) {
    return std::unexpected(record.error());
  }

  if (state_->frames || state_->lifecycle || state_->reclaiming) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  if (state_->count >= state_->options.max_nodes) {
    return std::unexpected(TreeError::NODE_LIMIT);
  }

  if ((*record)->depth >= state_->options.max_depth) {
    return std::unexpected(TreeError::DEPTH_LIMIT);
  }

  return {};
}

std::expected<NodeId, TreeError> WidgetTree::attach(NodeId parent, std::unique_ptr<Widget> widget, bool checked) {
  if (auto allowed = canInsert(parent); !allowed) {
    return std::unexpected(allowed.error());
  }

  auto &record = **state_->find(parent);
  auto slot = std::ranges::find_if(state_->slots, [](const State::Slot &entry) {
    return !entry.record && entry.generation != 0;
  });

  if (slot == state_->slots.end()) {
    return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
  }

  auto node_scope = scheduler().makeScope();

  if (!node_scope) {
    return std::unexpected(fromScheduler(node_scope.error()));
  }

  auto child = std::make_unique<State::Record>();

  child->id = {state_->session, static_cast<std::size_t>(slot - state_->slots.begin()), slot->generation};
  child->parent = &record;
  child->depth = record.depth + 1;
  child->widget = std::move(widget);
  child->scope = std::move(*node_scope);

  record.children.reserve(record.children.size() + 1);

  auto pointer = child.get();
  const auto id = pointer->id;

  record.children.push_back(std::move(child));
  slot->record = pointer;
  ++state_->count;
  state_->invalidate(Invalidation::LAYOUT);
  ++state_->lifecycle;

  bool mountFailed{};

  try {
    pointer->widget->mount();
  } catch (...) {
    mountFailed = true;
    report(id, TreeError::CALLBACK_FAILED);
  }

  --state_->lifecycle;

  if (checked && mountFailed) {
    // The enclosing borrow retains storage even when mount or the sink already removed this node.
    if (state_->find(id)) {
      (void)remove(Node<Widget>(id));
    }

    return std::unexpected(TreeError::CALLBACK_FAILED);
  }

  if (checked) {
    if (auto live = state_->find(id); !live) {
      return std::unexpected(live.error());
    }
  }

  return id;
}

std::expected<void, TreeError> WidgetTree::remove(Node<Widget> node) {
  auto record = state_->find(node.id());

  if (!record) {
    return std::unexpected(record.error());
  }

  if (state_->frames) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  Borrow guard(*this);

  // Revoke every descendant identity before closing scopes: releasing callback captures can
  // reenter the tree, but must never obtain a fresh borrow into the logically removed subtree.
  State::markDead(**record);
  state_->removal_pending = true;
  state_->invalidate(Invalidation::LAYOUT);
  State::closeScopes(**record);

  return {};
}

std::expected<void, TreeError> WidgetTree::close() {
  if (std::this_thread::get_id() != state_->owner) {
    return std::unexpected(TreeError::WRONG_THREAD);
  }

  if (state_->closed) {
    return {};
  }

  if (state_->frames) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  Borrow guard(*this);

  state_->closed = true;
  state_->removal_pending = true;

  if (state_->root) {
    State::markDead(*state_->root);
    State::closeScopes(*state_->root);
  }

  return {};
}

void WidgetTree::reclaim() noexcept {
  if (state_->reclaiming || !state_->removal_pending) {
    return;
  }

  state_->reclaiming = true;
  // Hooks can mark an already visited sibling dead. A repeated pass is bounded by
  // the initial node count: insertion is rejected throughout reclamation.
  auto sweep = [&](auto &&self, std::unique_ptr<State::Record> &node) -> void {
    if (!node) {
      return;
    }

    for (auto &child : node->children) {
      self(self, child);
    }

    std::erase_if(node->children, [](const auto &child) {
      return !child;
    });

    if (node->live) {
      return;
    }

    // A child unmount may remove this ancestor after its other children were visited.
    for (auto &child : node->children) {
      self(self, child);
    }

    node->children.clear();

    const auto id = node->id;

    {
      Borrow guard(*this);

      ++state_->lifecycle;

      try {
        node->widget->unmount();
      } catch (...) {
        report(id, TreeError::CALLBACK_FAILED);
      }

      --state_->lifecycle;
    }

    // Unpublish the address before running the widget destructor. Advance the generation only
    // after destruction; zero permanently retires an exhausted slot rather than reviving old IDs.
    auto &slot = state_->slots[id.slot];

    slot.record = nullptr;
    node.reset();
    slot.generation = nextGeneration(slot.generation);
    --state_->count;
  };
  bool pending;

  do {
    sweep(sweep, state_->root);
    pending = std::ranges::any_of(state_->slots, [](const State::Slot &slot) {
      return slot.record && !slot.record->live;
    });
  } while (pending);

  state_->removal_pending = false;
  state_->reclaiming = false;
}

const UiScope &WidgetTree::scope(NodeId id) const {
  return state_->slots[id.slot].record->scope;
}

Scheduler &WidgetTree::scheduler() const {
  return state_->scheduler;
}

std::expected<void, TreeError> WidgetTree::setVisible(Node<Widget> node, bool visible) {
  auto checked = access(node.id(), true);

  if (!checked) {
    return std::unexpected(checked.error());
  }

  auto &record = *state_->slots[node.id().slot].record;

  if (record.visible != visible) {
    record.visible = visible;
    state_->invalidate(Invalidation::LAYOUT);
  }

  return {};
}

std::expected<void, TreeError> WidgetTree::setEnabled(Node<Widget> node, bool enabled) {
  auto checked = access(node.id(), true);

  if (!checked) {
    return std::unexpected(checked.error());
  }

  auto &record = *state_->slots[node.id().slot].record;

  if (record.enabled != enabled) {
    record.enabled = enabled;
    state_->invalidate(Invalidation::PAINT);
  }

  return {};
}

std::expected<bool, TreeError> WidgetTree::inputEligible(Node<Widget> node) const {
  auto record = state_->find(node.id());

  if (!record) {
    return std::unexpected(record.error());
  }

  return State::shown(**record, true);
}

std::expected<void, TreeError> WidgetTree::invalidate(Node<Widget> node, Invalidation kind) {
  auto checked = access(node.id(), true);

  if (!checked) {
    return std::unexpected(checked.error());
  }

  if (kind != Invalidation::NONE && kind != Invalidation::PAINT && kind != Invalidation::LAYOUT) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  state_->invalidate(kind);

  return {};
}

std::expected<Invalidation, TreeError> WidgetTree::consumeFrameRequest() {
  if (auto checked = state_->check(); !checked) {
    return std::unexpected(checked.error());
  }

  if (state_->frames) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  if (auto *recorder = diagnostics(); recorder && recorder->enabled()) {
    if (state_->dirty == Invalidation::NONE && recorder->current().correlationId) {
      recorder->note(DiagnosticEvent::FRAME_NOT_REQUESTED, DiagnosticReason::FRAME_NOT_REQUESTED);
    } else if (state_->dirty != Invalidation::NONE) {
      state_->frame_diagnostic = state_->pending_diagnostic;
      state_->pending_diagnostic = {};
    }
  }

  return std::exchange(state_->dirty, Invalidation::NONE);
}

Diagnostics *WidgetTree::diagnostics() const noexcept {
  return state_->scheduler.diagnostics();
}

DiagnosticContext WidgetTree::diagnosticFrame() const noexcept {
  auto *recorder = diagnostics();

  return recorder && recorder->enabled() ? state_->frame_diagnostic : DiagnosticContext{};
}

std::expected<void, TreeError> WidgetTree::beginFrame() const {
  if (auto checked = state_->check(); !checked) {
    return checked;
  }

  if (state_->borrows || state_->reclaiming) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  return {};
}

std::expected<CellSize, TreeError> WidgetTree::measure(Node<Widget> node, CellSize available) {
  auto record = state_->find(node.id());

  if (!record) {
    return std::unexpected(record.error());
  }

  if (state_->extension) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  if (!State::shown(**record, false)) {
    return CellSize{};
  }

  Borrow guard(*this, true);

  state_->extension = true;

  std::expected<CellSize, TreeError> result;

  try {
    result = (*record)->widget->measure(available.normalized());
  } catch (...) {
    result = std::unexpected(TreeError::CALLBACK_FAILED);
  }

  state_->extension = false;

  if (!result) {
    report(node.id(), result.error());
  } else {
    *result = result->normalized();

    if (const auto &style = (*record)->widget->computedStyle()) {
      result->width = std::max(result->width, style->minWidth);
    }
  }

  return result;
}

std::expected<void, TreeError> WidgetTree::arrange(Node<Widget> node, CellRect local, CellRect viewport) {
  auto record = state_->find(node.id());

  if (!record) {
    return std::unexpected(record.error());
  }

  if (state_->extension) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  auto &value = **record;

  // A parent rearrangement invalidates descendant surface coordinates too. Publish fresh
  // geometry only after the hook succeeds; failures leave hit-testing unable to use stale cells.
  State::invalidateGeometry(value);

  if (!State::shown(value, false)) {
    return {};
  }

  CellRect surface = local;
  CellRect clip = viewport;

  if (value.parent) {
    if (!value.parent->geometry.valid) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    auto translated = local.translated(value.parent->geometry.surface.origin());

    if (!translated) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    surface = *translated;
    clip = value.parent->geometry.clip;
  }

  Borrow guard(*this, true);

  state_->extension = true;

  std::expected<void, TreeError> result;

  try {
    result = value.widget->arrange(local);
  } catch (...) {
    result = std::unexpected(TreeError::CALLBACK_FAILED);
  }

  state_->extension = false;

  if (!result) {
    report(node.id(), result.error());

    return result;
  }

  value.geometry = {local, surface, surface.intersection(clip), true};
  state_->invalidate(Invalidation::PAINT);

  return {};
}

std::expected<void, TreeError> WidgetTree::paint(Node<Widget> node, PaintContext &context) {
  auto record = state_->find(node.id());

  if (!record) {
    return std::unexpected(record.error());
  }

  if (!state_->frames || state_->extension) {
    return std::unexpected(TreeError::WOULD_BLOCK);
  }

  if (!State::shown(**record, false)) {
    return {};
  }

  if (!(*record)->geometry.valid) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  if ((*record)->geometry.clip.empty()) {
    return {};
  }

  Borrow guard(*this, true);

  state_->extension = true;

  std::expected<void, TreeError> result;

  try {
    result = (*record)->widget->paint(context);
  } catch (...) {
    result = std::unexpected(TreeError::CALLBACK_FAILED);
  }

  state_->extension = false;

  if (!result) {
    report(node.id(), result.error());
  }

  return result;
}
} // namespace portals
