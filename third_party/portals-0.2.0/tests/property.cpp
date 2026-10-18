// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/property.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

namespace allocation_test {
thread_local std::ptrdiff_t remaining = -1;
}

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *p = std::malloc(size ? size : 1)) {
    return p;
  }

  throw std::bad_alloc();
}

void operator delete(void *p) noexcept {
  std::free(p);
}

void operator delete(void *p, std::size_t) noexcept {
  std::free(p);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *p) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, std::size_t) noexcept {
  ::operator delete(p);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new[](size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *p, const std::nothrow_t &) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept {
  ::operator delete[](p);
}

namespace {
using namespace portals;
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> value) {
  if (!value) {
    std::cerr << "fixture failed\n";
    std::abort();
  }

  return std::move(*value);
}

struct Counters {
  int measure{}, arrange{}, paint{};
};

struct Probe : Widget {
  Counters &counts;

  explicit Probe(Counters &value) : counts(value) {
  }

  std::expected<CellSize, TreeError> measure(CellSize available) override {
    ++counts.measure;

    return available;
  }

  std::expected<void, TreeError> arrange(CellRect) override {
    ++counts.arrange;

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &) override {
    ++counts.paint;

    return {};
  }
};

struct Sink : PaintContext {
  std::expected<void, TreeError> submit(std::span<const RenderCommand>) override {
    return {};
  }
};

void invalidationAndSnapshots() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  Counters counts;
  auto node = take(tree->emplace<Probe>(root, counts));
  auto color = take(Property<int>::create(*tree, node, 1, Invalidation::PAINT));
  auto padding = take(Property<int>::create(*tree, node, 0, Invalidation::LAYOUT));
  std::vector<std::uint64_t> revisions;
  auto connection = take(color->observe(node, [&](Probe &, auto event) {
    revisions.push_back(event->revision);
  }));
  Sink sink;
  auto frame = [&] {
    auto dirty = take(tree->consumeFrameRequest());

    check(
      tree
        ->frame([&] {
          if (dirty == Invalidation::LAYOUT) {
            take(tree->measure(node, {8, 3}));
            check(tree->arrange(root, take(CellRect::from({}, {8, 3})), take(CellRect::from({}, {8, 3}))).has_value(),
                  "arrange root");
            check(tree->arrange(node, take(CellRect::from({}, {8, 3})), take(CellRect::from({}, {8, 3}))).has_value(),
                  "arrange child");
          }

          if (dirty != Invalidation::NONE) {
            check(tree->paint(node, sink).has_value(), "paint");
          }
        })
        .has_value(),
      "frame");
  };

  frame();
  check(counts.measure == 1 && counts.arrange == 1 && counts.paint == 1, "initial frame");

  take(tree->consumeFrameRequest());

  auto equal = take(color->set(1));

  check(!equal.changed && equal.snapshot->revision == 0, "equal no revision");
  check(take(tree->consumeFrameRequest()) == Invalidation::NONE, "equal no frame");
  check(scheduler->metrics().data.accepted == 0, "equal no publish");

  auto retained = take(color->snapshot());
  auto update = take(color->set(2));

  check(update.changed && update.accepted == 1 && update.rejected == 0, "changed accepted");
  check(retained->value == 1 && retained->revision == 0, "snapshot owns old value");

  frame();
  check(counts.measure == 1 && counts.arrange == 1 && counts.paint == 2, "color only paints");

  take(scheduler->dispatch());
  check(revisions == std::vector<std::uint64_t>{1}, "owning event revision");

  take(padding->set(2));
  frame();
  check(counts.measure == 2 && counts.arrange == 2 && counts.paint == 3, "padding layout and paint");
  check(tree
          ->frame([&] {
            auto denied = color->set(9);

            check(!denied && denied.error() == TreeError::WOULD_BLOCK, "set forbidden during frame");
          })
          .has_value(),
        "frame mutation guard");
  check(take(color->snapshot())->value == 2, "frame rejection retains value");
}

void reentrantOrder() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto node = take(tree->root());
  auto property = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT));

  std::vector<int> seen;
  int depth{}, maximum{};
  auto first = take(property->observe(node, [&](Widget &, auto event) {
    ++depth;
    maximum = std::max(maximum, depth);
    seen.push_back(10 + event->value);

    if (event->value == 1) {
      take(property->set(2));
    }

    --depth;
  }));
  auto second = take(property->observe(node, [&](Widget &, auto event) {
    seen.push_back(20 + event->value);
  }));

  take(property->set(1));
  take(scheduler->dispatch());
  check(seen == std::vector<int>{11, 21, 12, 22}, "revision order across observers");
  check(maximum == 1, "no synchronous recursion");
}

void backpressure() {
  SchedulerOptions options;

  options.data_capacity = 1;

  auto scheduler = take(Scheduler::create(options));
  auto tree = take(WidgetTree::create(*scheduler));
  auto node = take(tree->root());
  auto property = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT));

  std::vector<int> seen;
  auto first = take(property->observe(node, [&](Widget &, auto event) {
    seen.push_back(event->value);
  }));
  auto second = take(property->observe(node, [&](Widget &, auto event) {
    seen.push_back(10 + event->value);
  }));

  take(tree->consumeFrameRequest());

  auto partial = take(property->set(1));

  check(partial.changed && partial.accepted == 1 && partial.rejected == 1, "explicit partial commit");
  check(partial.notification_error == SchedulerError::WOULD_BLOCK, "partial reason");
  check(take(tree->consumeFrameRequest()) == Invalidation::PAINT, "partial commit still invalidates");

  auto full = take(property->set(2));

  check(full.changed && full.accepted == 0 && full.rejected == 2, "full queue still commits");
  check(take(property->snapshot())->value == 2, "full queue current snapshot");
  check(take(tree->consumeFrameRequest()) == Invalidation::PAINT, "full queue still invalidates");

  auto equal = take(property->set(2));

  check(!equal.changed && equal.accepted == 0 && equal.rejected == 0, "equal does not retry rejected events");

  take(scheduler->dispatch());
  check(seen == std::vector<int>{1}, "accepted old event retains its own value");
  check(scheduler->metrics().data.high_water == 1, "bounded queue");
}

void cancellation() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto property = take(Property<int>::create(*tree, root, 0, Invalidation::PAINT, {1}));
  int calls{};
  Subscription self;

  self = take(property->observe(root, [&](Widget &, auto) {
    ++calls;
    check(self.disconnect().has_value(), "self disconnect");

    auto update = take(property->set(2));

    check(update.cancelled == 1 && update.notification_error == SchedulerError::CLOSED, "closed endpoint receipt");

    auto replacement = property->observe(root, [](Widget &, auto) {
    });

    check(!replacement && replacement.error() == TreeError::NODE_LIMIT, "executing observer retains its slot");
  }));

  auto limited = property->observe(root, [](Widget &, auto) {
  });

  check(!limited && limited.error() == TreeError::NODE_LIMIT, "finite observer capacity");

  take(property->set(1));
  take(scheduler->dispatch());
  check(calls == 1, "self disconnect suppresses later event");

  auto reused = take(property->observe(root, [&](Widget &, auto) {
    ++calls;
  }));

  take(property->set(3));
  property.reset();

  take(scheduler->dispatch());
  check(calls == 1, "source destruction cancels queued callbacks");

  property = take(Property<int>::create(*tree, root, 0, Invalidation::PAINT));

  auto first = take(property->observe(root, [&](Widget &, auto) {
    property.reset();
    ++calls;
  }));
  auto second = take(property->observe(root, [&](Widget &, auto) {
    ++calls;
  }));

  take(property->set(1));
  take(scheduler->dispatch());
  check(calls == 2, "source destroy during dispatch cancels remaining observers");
}

void nodeLifetime() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto parent = take(tree->emplace<Widget>(root));
  auto child = take(tree->emplace<Widget>(parent));

  auto property = take(Property<int>::create(*tree, root, 0, Invalidation::PAINT));
  int calls{};
  auto first = take(property->observe(child, [&](Widget &, auto) {
    ++calls;
    check(tree->remove(parent).has_value(), "remove parent during current child callback");
  }));
  auto second = take(property->observe(child, [&](Widget &, auto) {
    ++calls;
  }));

  take(property->set(1));
  take(scheduler->dispatch());
  check(calls == 1, "removed subtree receives no late callbacks");

  auto target = take(tree->emplace<Widget>(root));
  auto target_property = take(Property<int>::create(*tree, target, 5, Invalidation::PAINT));

  check(tree->remove(target).has_value(), "remove property target");

  auto denied = target_property->set(6);

  check(!denied && denied.error() == TreeError::STALE_NODE, "stale target rejects before commit");
  check(take(target_property->snapshot())->value == 5, "stale target preserves value");

  auto closed = take(property->observe(root, [&](Widget &, auto) {
    ++calls;
  }));

  take(property->set(2));
  check(tree->close().has_value(), "close all node scopes");

  take(scheduler->dispatch());
  check(calls == 1, "no callbacks after scope close");
}

void affinityAndSession() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto other = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto foreign = take(other->root());
  auto invalid = Property<int>::create(*tree, foreign, 0, Invalidation::PAINT);

  check(!invalid && invalid.error() == TreeError::WRONG_SESSION, "foreign source target");

  auto property = take(Property<int>::create(*tree, root, 0, Invalidation::PAINT));
  auto observer = property->observe(foreign, [](Widget &, auto) {
  });

  check(!observer && observer.error() == TreeError::WRONG_SESSION, "foreign receiver");

  bool rejected{};
  std::thread worker([&] {
    auto set = property->set(1);
    auto snapshot = property->snapshot();
    auto observe = property->observe(root, [](Widget &, auto) {
    });

    rejected = !set && set.error() == TreeError::WRONG_THREAD && !snapshot &&
               snapshot.error() == TreeError::WRONG_THREAD && !observe && observe.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(rejected, "all property access uses owner thread");
}

struct Reenter {
  std::function<void()> action;

  ~Reenter() {
    if (action) {
      action();
    }
  }
};

struct Payload {
  int value{};
  std::shared_ptr<Reenter> release;

  bool operator==(const Payload &other) const {
    return value == other.value;
  }
};

void destructorReentry() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  std::unique_ptr<Property<Payload>> property;
  auto release = std::make_shared<Reenter>();

  release->action = [&] {
    take(property->set(Payload{2, {}}));
  };
  property = take(Property<Payload>::create(*tree, root, Payload{0, release}, Invalidation::PAINT));
  release.reset();

  std::vector<int> seen;
  auto first = take(property->observe(root, [&](Widget &, auto event) {
    seen.push_back(10 + event->value.value);
  }));
  auto second = take(property->observe(root, [&](Widget &, auto event) {
    seen.push_back(20 + event->value.value);
  }));

  auto result = take(property->set(Payload{1, {}}));

  check(result.snapshot->value.value == 1 && result.accepted == 2,
        "receipt describes original commit after destructor reentry");
  check(take(property->snapshot())->value.value == 2, "old value destructor can update source");

  take(scheduler->dispatch());
  check(seen == std::vector<int>{11, 21, 12, 22}, "old value released after all publication attempts");

  auto captured = std::make_shared<Reenter>();

  captured->action = [&] {
    take(property->set(Payload{3, {}}));
  };

  auto receiver = take(property->observe(root, [captured](Widget &, auto) {
  }));

  captured.reset();
  check(receiver.disconnect().has_value(), "disconnect releases capture with reentry");

  take(scheduler->dispatch());
  check(seen == std::vector<int>{11, 21, 12, 22, 13, 23}, "capture destructor reentry leaves endpoint iteration valid");
}

struct RegistrationReentry {
  bool fired{};
  std::function<void()> on_destroy;
};

struct ReentrantObserver {
  std::shared_ptr<RegistrationReentry> state;
  bool moved_from{};

  explicit ReentrantObserver(std::shared_ptr<RegistrationReentry> value) : state(std::move(value)) {
  }

  ReentrantObserver(const ReentrantObserver &) = default;

  ReentrantObserver(ReentrantObserver &&other) : state(other.state) {
    other.moved_from = true;
  }

  ~ReentrantObserver() {
    if (moved_from && !state->fired) {
      state->fired = true;
      state->on_destroy();
    }
  }

  void operator()(Widget &, std::shared_ptr<const PropertyChange<int>>) {
  }
};

void registrationDestructorReentry() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto node = take(tree->root());
  auto property = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT, {1}));
  auto state = std::make_shared<RegistrationReentry>();
  Subscription inner;
  int calls{};

  state->on_destroy = [&] {
    inner = take(property->observe(node, [&](Widget &, auto) {
      ++calls;
    }));
  };

  ReentrantObserver callback(state);
  auto outer = property->observe(node, std::move(callback));
  auto third = property->observe(node, [](Widget &, auto) {
  });

  check(state->fired, "moved-from observer destructor reenters during registration");
  check(!outer && outer.error() == TreeError::NODE_LIMIT, "reentrant registration retains the observer limit");
  check(!third && third.error() == TreeError::NODE_LIMIT, "observer limit remains effective after reentry");

  auto update = take(property->set(1));

  take(scheduler->dispatch());
  check(update.accepted == 1 && calls == 1, "only the admitted nested observer receives publication");

  auto closing = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT));
  auto close_state = std::make_shared<RegistrationReentry>();

  close_state->on_destroy = [&] {
    closing.reset();
  };

  ReentrantObserver closing_callback(close_state);
  auto closed = closing->observe(node, std::move(closing_callback));

  check(close_state->fired && !closing, "observer temporary can destroy the source");
  check(!closed && closed.error() == TreeError::CLOSED, "source closure during registration is observable");
}

void allocationFailures() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  bool eventually_created{};
  constexpr std::ptrdiff_t ALLOCATION_PROBE_LIMIT = 20;

  for (std::ptrdiff_t budget = 0; budget < ALLOCATION_PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    auto created = Property<int>::create(*tree, root, 0, Invalidation::PAINT);

    allocation_test::remaining = -1;

    if (created) {
      eventually_created = true;

      break;
    }

    check(created.error() == TreeError::NO_MEMORY, "factory allocation failure is observable");
  }

  check(eventually_created, "factory succeeds after finite allocations");

  auto property = take(Property<int>::create(*tree, root, 0, Invalidation::PAINT, {1}));

  take(tree->consumeFrameRequest());
  allocation_test::remaining = 0;

  auto denied = property->set(1);

  allocation_test::remaining = -1;
  check(!denied && denied.error() == TreeError::NO_MEMORY, "set allocation failure");
  check(take(property->snapshot())->revision == 0, "allocation failure leaves revision");
  check(take(tree->consumeFrameRequest()) == Invalidation::NONE, "allocation failure leaves frame");

  bool eventually_observed{};

  for (std::ptrdiff_t budget = 0; budget < ALLOCATION_PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    auto registered = property->observe(root, [](Widget &, auto) {
    });

    allocation_test::remaining = -1;

    if (registered) {
      eventually_observed = true;

      break;
    }

    check(registered.error() == TreeError::NO_MEMORY, "observe allocation failure without capacity leak");
  }

  check(eventually_observed, "observe succeeds after finite allocations");
}

void stringAllocationFailures() {
  constexpr std::ptrdiff_t MAX_BUDGET = 160;
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  const auto root = take(tree->root());
  const std::string initial(80, 'a');
  const std::string candidate(80, 'b');

  for (const bool creating : {true, false}) {
    bool succeeded{};
    bool observedFailure{};

    for (std::ptrdiff_t budget = 0; budget < MAX_BUDGET; ++budget) {
      auto property = take(Property<std::string>::create(*tree, root, initial, Invalidation::PAINT));
      std::expected<std::unique_ptr<Property<std::string>>, TreeError> created;
      std::expected<PropertyUpdate<std::string>, TreeError> changed;
      bool argumentFailed{};
      int notifications{};
      auto observer = take(property->observe(root, [&](Widget &, auto) {
        ++notifications;
      }));

      take(tree->consumeFrameRequest());
      allocation_test::remaining = budget;

      try {
        if (creating) {
          created = Property<std::string>::create(*tree, root, candidate, Invalidation::PAINT);
        } else {
          changed = property->set(candidate);
        }
      } catch (const std::bad_alloc &) {
        argumentFailed = true;
      }

      allocation_test::remaining = -1;

      check(!argumentFailed, "const-reference string operations contain allocation failures inside their boundary");

      if (!argumentFailed && (creating ? bool(created) : bool(changed))) {
        succeeded = true;

        if (creating) {
          check(take((*created)->snapshot())->value == candidate, "factory owns complete string snapshot");
        } else {
          const auto current = take(property->snapshot());

          check(current == changed->snapshot && current->value == candidate && current->revision == 1,
                "string update receipt is the committed snapshot");
          take(scheduler->dispatch());
          check(notifications == 1, "successful string update notifies once");
        }

        break;
      }

      observedFailure = true;
      check(argumentFailed || (creating ? created.error() : changed.error()) == TreeError::NO_MEMORY,
            "string argument allocation throws and internal allocation returns NO_MEMORY");
      check(take(property->snapshot())->value == initial && take(property->snapshot())->revision == 0,
            "failed string construction preserves previous value and revision");
      check(take(tree->consumeFrameRequest()) == Invalidation::NONE, "failed string construction does not invalidate");
      take(scheduler->dispatch());
      check(notifications == 0, "failed string construction does not enqueue notifications");
    }

    check(succeeded && observedFailure, "string create/set sweep observes failure and eventual success");
  }
}

struct MoveOnlyValue {
  int value{};

  explicit MoveOnlyValue(int initial) : value(initial) {
  }

  MoveOnlyValue(const MoveOnlyValue &) = delete;
  MoveOnlyValue(MoveOnlyValue &&) = default;
  MoveOnlyValue &operator=(MoveOnlyValue &&) = default;
  bool operator==(const MoveOnlyValue &) const = default;
};

void moveOnlySnapshots() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  const auto root = take(tree->root());
  auto property = take(Property<MoveOnlyValue>::create(*tree, root, MoveOnlyValue{1}, Invalidation::PAINT));
  const auto previous = take(property->snapshot());
  const auto changed = take(property->set(MoveOnlyValue{2}));

  check(previous->value.value == 1 && changed.snapshot->value.value == 2 && changed.snapshot->revision == 1,
        "move-only values retain generic snapshot support and immutable previous owners");
}

void moveOnlyContainerSnapshots() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  const auto root = take(tree->root());
  std::vector<std::unique_ptr<int>> initial(0);

  initial.push_back(std::make_unique<int>(1));

  auto property =
    take(Property<std::vector<std::unique_ptr<int>>>::create(*tree, root, std::move(initial), Invalidation::PAINT));
  const auto previous = take(property->snapshot());
  std::vector<std::unique_ptr<int>> candidate(0);

  candidate.push_back(std::make_unique<int>(2));

  const auto changed = take(property->set(std::move(candidate)));

  check(*previous->value[0] == 1 && *changed.snapshot->value[0] == 2,
        "container with a false-positive copy trait still uses its valid move path");
}

struct SourceWidget : Widget {
  bool &destroyed;
  std::unique_ptr<Property<int>> property;

  explicit SourceWidget(bool &flag) : destroyed(flag) {
  }

  ~SourceWidget() override {
    destroyed = true;
  }
};

void currentTargetDestruction() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  bool destroyed{};
  int calls{};
  auto node = take(tree->emplace<SourceWidget>(root, destroyed));
  Subscription first, second;

  check(tree
          ->edit(node,
                 [&](SourceWidget &widget) {
                   widget.property = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT));
                   first = take(widget.property->observe(node, [&](SourceWidget &, auto event) {
                     ++calls;
                     check(tree->remove(node).has_value(), "remove current target");
                     check(!destroyed && event->value == 1, "active widget and snapshot survive self removal");
                   }));
                   second = take(widget.property->observe(node, [&](SourceWidget &, auto) {
                     ++calls;
                   }));
                   take(widget.property->set(1));
                 })
          .has_value(),
        "initialize node-owned source");

  take(scheduler->dispatch());
  check(destroyed && calls == 1, "node-owned source reclaimed after callback without late invocation");
}

void boundedReentry() {
  SchedulerOptions options;

  options.data_budget = 3;

  auto scheduler = take(Scheduler::create(options));
  auto tree = take(WidgetTree::create(*scheduler));
  auto node = take(tree->root());
  auto property = take(Property<int>::create(*tree, node, 0, Invalidation::PAINT));
  int calls{};
  auto token = take(property->observe(node, [&](Widget &, auto event) {
    ++calls;
    take(property->set(event->value + 1));
  }));

  take(property->set(1));

  auto dispatched = take(scheduler->dispatch());

  check(calls == 3 && dispatched.data == 3, "self-updating callback respects dispatch budget");
  check(scheduler->metrics().data.pending == 1, "self-updating callback leaves bounded continuation");
  check(token.disconnect().has_value(), "cancel self-updating observer");
}
} // namespace

int main() {
  invalidationAndSnapshots();
  reentrantOrder();
  backpressure();
  cancellation();
  nodeLifetime();
  affinityAndSession();
  destructorReentry();
  registrationDestructorReentry();
  allocationFailures();
  stringAllocationFailures();
  moveOnlySnapshots();
  moveOnlyContainerSnapshots();
  currentTargetDestruction();
  boundedReentry();

  return failures ? 1 : 0;
}
