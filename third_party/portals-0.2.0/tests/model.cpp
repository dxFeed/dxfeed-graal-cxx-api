// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/model.hpp>

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>

namespace allocation_test {
thread_local std::size_t count{};
thread_local std::size_t bytes{};
thread_local std::ptrdiff_t remaining{-1};
} // namespace allocation_test

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  ++allocation_test::count;
  allocation_test::bytes += size;

  if (void *value = std::malloc(size ? size : 1)) {
    return value;
  }

  throw std::bad_alloc();
}

void operator delete(void *value) noexcept {
  std::free(value);
}

void operator delete(void *value, std::size_t) noexcept {
  std::free(value);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *value) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, std::size_t) noexcept {
  ::operator delete(value);
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
    std::cerr << "fixture unexpectedly rejected\n";
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*value);
  }
}

struct Page : ModelSnapshot<int> {
  std::array<ModelRow<int>, 2> values;
  RowRange available;
  RangeState readiness{RangeState::READY};
  mutable int reads{};

  Page(std::uint64_t revision, RowRange range, int value, std::uint64_t count = 1000000)
      : ModelSnapshot(revision, count), values{{{{42, 1}, value}, {{43, 1}, value + 1}}}, available(range) {
  }

  ModelRows<int> read(RowRange range) const override {
    ++reads;

    if (readiness == RangeState::FAILED) {
      return {RangeState::FAILED, {}, "provider failed"};
    }

    if (range.first >= available.first && range.first - available.first <= available.count &&
        range.count <= available.count - (range.first - available.first)) {
      return {RangeState::READY,
              std::span(values).subspan(static_cast<std::size_t>(range.first - available.first), range.count),
              {}};
    }

    return {};
  }
};

struct Fixture {
  std::atomic<SchedulerTime> now{};

  static SchedulerTime clock(void *context) noexcept {
    return static_cast<std::atomic<SchedulerTime> *>(context)->load();
  }

  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create({}, {clock, &now}));
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
  std::unique_ptr<Model<int>> source;

  explicit Fixture(ModelOptions options = {})
      : source(take(Model<int>::create(*tree, root, std::make_shared<const Page>(1, RowRange{0, 2}, 10), options))) {
  }
};

void independentRangesAndFrame() {
  Fixture f({2, 3, 4});
  auto a = take(f.source->openRange());
  auto b = take(f.source->openRange());
  auto first = take(a.request({10, 2}));
  auto second = take(b.request({20, 2}));
  auto retained = take(f.source->pin());

  check(take(retained.range(a.id())).state() == RangeState::PENDING, "pending range is explicit");

  bool published{};

  check(f.tree
          ->frame([&] {
            auto left = retained;
            std::thread worker([&] {
              published = second.complete(std::make_shared<const Page>(1, RowRange{20, 2}, 20)).has_value() &&
                          first.complete(std::make_shared<const Page>(1, RowRange{10, 2}, 30)).has_value();
            });

            worker.join();

            auto right = retained;

            check(left.revision() == right.revision() && left.dataRevision() == right.dataRevision(),
                  "two views pin one frame revision during worker publication");
            check(take(left.range(a.id())).state() == RangeState::PENDING &&
                    take(right.range(b.id())).state() == RangeState::PENDING,
                  "neither view observes queued completions during frame");

            auto denied = a.request({0, 2});

            check(!denied && denied.error() == TreeError::WOULD_BLOCK, "frame cannot initiate work");
          })
          .has_value(),
        "real tree frame");
  check(published, "workers queue both independent completions");
  f.now.store(17);
  take(f.tree->consumeFrameRequest());
  take(f.scheduler->dispatch());

  auto frame = take(f.source->pin());

  check(f.scheduler->metrics().data.last_latency == 17, "injected clock measures deferred UI delivery");
  check(take(frame.range(a.id())).rows()[0].value == 30, "out-of-order independent range retained");
  check(take(frame.range(b.id())).rows()[0].value == 20, "second range retained");
  check(take(retained.range(a.id())).state() == RangeState::PENDING, "retained frame never mutates");
  check(take(f.tree->consumeFrameRequest()) == Invalidation::LAYOUT, "range readiness invalidates context");

  auto borrowed = take(frame.range(a.id()));

  f.source.reset();
  check(borrowed.rows()[0].value == 30, "range keeps provider storage alive after source destruction");
}

void retainedRangeIsSoleStorageOwner() {
  std::optional<ModelRange<int>> retained;
  std::weak_ptr<const Page> storage;

  {
    Fixture f;
    auto consumer = take(f.source->openRange());
    auto request = take(consumer.request({10, 2}));
    auto page = std::make_shared<const Page>(1, RowRange{10, 2}, 10);

    storage = page;
    take(request.complete(page));
    page.reset();
    take(f.scheduler->dispatch());
    retained = take(take(f.source->pin()).range(consumer.id()));
  }

  check(!storage.expired() && retained->rows()[0].value == 10,
        "range alone owns storage after source, frame, worker and view context are destroyed");
  retained.reset();
  check(storage.expired(), "storage releases after the final retained range");
}

void equalOverlappingAndGenerations() {
  Fixture f;
  auto a = take(f.source->openRange());
  auto b = take(f.source->openRange());
  auto same_a = take(a.request({10, 2}));
  auto same_b = take(b.request({10, 2}));
  auto old_id = a.id();

  take(a.close());
  check(same_a.stopRequested() && !same_b.stopRequested(), "equal ranges have independent cancellation");
  check(!same_a.complete(std::make_shared<const Page>(1, RowRange{10, 2}, 10)), "closed consumer cannot publish");
  take(same_b.complete(std::make_shared<const Page>(1, RowRange{10, 2}, 10)));
  take(f.scheduler->dispatch());
  check(take(take(f.source->pin()).range(b.id())).rows()[0].value == 10, "closing equal peer preserves live result");
  a = take(f.source->openRange());
  check(a.id() != old_id, "consumer identity never reused");

  auto overlapping_a = take(a.request({10, 2}));
  auto overlapping_b = take(b.request({11, 1}));

  take(overlapping_b.complete(std::make_shared<const Page>(1, RowRange{10, 2}, 10)));
  take(overlapping_a.complete(std::make_shared<const Page>(1, RowRange{10, 2}, 10)));
  take(f.scheduler->dispatch());

  auto frame = take(f.source->pin());

  check(take(frame.range(a.id())).rows()[1].id == take(frame.range(b.id())).rows()[0].id,
        "overlap agrees on stable row identity");
  check(take(frame.range(b.id())).rows().size() == 1, "overlap does not spatially merge consumers");

  auto older = take(a.request({30, 2}));

  take(older.complete(std::make_shared<const Page>(1, RowRange{30, 2}, 30)));

  auto newer = take(a.request({40, 2}));

  take(newer.complete(std::make_shared<const Page>(1, RowRange{40, 2}, 40)));
  take(f.scheduler->dispatch());
  check(take(take(f.source->pin()).range(a.id())).rows()[0].value == 40,
        "queued old generation cannot replace newer request");
  check(older.stopRequested(), "superseded generation cooperatively cancelled");
}

void revisionErrorAndLifetime() {
  Fixture f;
  auto consumer = take(f.source->openRange());
  auto old = take(consumer.request({0, 2}));
  auto publisher = f.source->publisher();

  take(publisher.publish(std::make_shared<const Page>(3, RowRange{0, 2}, 30)));
  take(old.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  take(publisher.publish(std::make_shared<const Page>(2, RowRange{0, 2}, 20)));
  take(f.scheduler->dispatch());

  auto frame = take(f.source->pin());

  check(frame.dataRevision() == 3 && take(frame.read({0, 2})).rows()[0].value == 30,
        "full snapshots apply monotonically");
  check(old.stopRequested() && take(frame.range(consumer.id())).state() == RangeState::PENDING,
        "new revision cancels old work and clears readiness");

  auto request = take(consumer.request({0, 2}));
  auto error = std::make_shared<Page>(3, RowRange{0, 2}, 0);

  error->readiness = RangeState::FAILED;
  take(request.complete(error));
  take(f.scheduler->dispatch());

  auto result = take(take(f.source->pin()).range(consumer.id()));

  check(result.state() == RangeState::FAILED && result.error() == "provider failed",
        "error is distinct from pending and ready");

  auto stale = request.complete(std::make_shared<const Page>(2, RowRange{0, 2}, 20));

  check(!stale && stale.error() == SchedulerError::INVALID_ARGUMENT, "wrong data revision rejected before queue");

  auto duplicate = request.complete(error);

  check(!duplicate && duplicate.error() == SchedulerError::CLOSED, "one successful completion per generation");
  take(f.tree->close());

  auto late = publisher.publish(std::make_shared<const Page>(4, RowRange{0, 2}, 40));

  check(!late && late.error() == SchedulerError::CLOSED, "context closure rejects late full publication");
  f.source.reset();
  f.tree.reset();
  f.scheduler.reset();
  check(result.error() == "provider failed" && request.stopRequested(),
        "retained error owner survives full context teardown");
}

struct ReleaseAction {
  std::function<void()> action;

  ~ReleaseAction() {
    action();
  }
};

void boundedWorkAndReentry() {
  Fixture f({2, 2, 1});
  auto a = take(f.source->openRange());
  auto b = take(f.source->openRange());
  auto denied_consumer = f.source->openRange();

  check(!denied_consumer && denied_consumer.error() == TreeError::NODE_LIMIT, "finite consumer slots");

  auto invalid = a.request({999999, 2});
  auto oversized = a.request({0, 3});

  check(!invalid && !oversized, "bounds checked before work");

  bool reentered{};
  auto storage = std::make_shared<ReleaseAction>();

  storage->action = [&] {
    auto denied = b.request({0, 2});

    reentered = !denied && denied.error() == TreeError::WOULD_BLOCK;
  };

  std::weak_ptr<ReleaseAction> retained = storage;
  auto request = take(a.request({0, 2}, storage));

  storage.reset();
  request.cancel();

  auto blocked = b.request({0, 2});

  check(!blocked && blocked.error() == TreeError::WOULD_BLOCK && !retained.expired(),
        "cancelled live worker still consumes budget and owns data");
  request = {};
  check(reentered && retained.expired(), "last-owner destructor reentry cannot oversubscribe work limit");

  auto next = take(b.request({0, 2}));

  take(next.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  next = {};
  blocked = a.request({0, 2});
  check(!blocked && blocked.error() == TreeError::WOULD_BLOCK, "queued completion retains request reservation");
  take(f.scheduler->dispatch());

  auto reusable = take(a.request({0, 2}));

  take(a.close());
  check(reusable.stopRequested(), "consumer close cancels live worker");
  a = take(f.source->openRange());
  check(a.id() != b.id(), "reused capacity has fresh identity");
}

void mailboxBackpressureAndAffinity() {
  SchedulerOptions options;

  options.data_capacity = 1;

  auto scheduler = take(Scheduler::create(options));
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto model = take(Model<int>::create(*tree, root, std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  auto a = take(model->openRange());
  auto b = take(model->openRange());
  auto first = take(a.request({0, 2}));
  auto second = take(b.request({0, 2}));

  take(first.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10)));

  auto rejected = second.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10));

  check(!rejected && rejected.error() == SchedulerError::WOULD_BLOCK, "mailbox overflow observable");
  take(scheduler->dispatch());
  take(second.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  take(scheduler->dispatch());
  check(take(take(model->pin()).range(b.id())).state() == RangeState::READY, "rejected completion can retry");

  bool affinity{};
  std::thread worker([&] {
    auto pin = model->pin();
    auto request = a.request({0, 2});
    auto close = a.close();

    affinity = !pin && pin.error() == TreeError::WRONG_THREAD && !request &&
               request.error() == TreeError::WRONG_THREAD && !close && close.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(affinity, "UI operations reject foreign threads");

  auto work = take(a.request({0, 2}));

  take(tree->remove(root));

  auto closed = work.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10));

  check(!closed && closed.error() == SchedulerError::CLOSED, "destroyed context suppresses range completion");
}

struct Counted {
  static inline int copies{};
  int value{};

  explicit Counted(int v = 0) : value(v) {
  }

  Counted(const Counted &other) : value(other.value) {
    ++copies;
  }

  Counted &operator=(const Counted &) = default;
};

struct Synthetic : ModelSnapshot<Counted> {
  std::array<ModelRow<Counted>, 2> rows{{{{42, 1}, Counted(1)}, {{43, 1}, Counted(2)}}};

  Synthetic(std::uint64_t revision, std::uint64_t count) : ModelSnapshot(revision, count) {
  }

  ModelRows<Counted> read(RowRange range) const override {
    if (range.first == 0 && range.count == 2) {
      return {RangeState::READY, rows, {}};
    }

    return {};
  }
};

std::pair<std::size_t, std::size_t> publicationCost(std::uint64_t count) {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto model = take(Model<Counted>::create(*tree, take(tree->root()), std::make_shared<const Synthetic>(1, count)));
  auto publisher = model->publisher();

  Counted::copies = 0;

  auto before = allocation_test::count;
  auto bytes = allocation_test::bytes;

  for (std::uint64_t revision = 2; revision != 12; ++revision) {
    take(publisher.publish(std::make_shared<const Synthetic>(revision, count)));
    take(scheduler->dispatch());

    auto pin_start = allocation_test::count;
    auto frame = take(model->pin());
    auto range = take(frame.read({0, 2}));

    check(frame.rowCount() == count && range.rows()[0].value.value == 1, "synthetic visible page");
    check(allocation_test::count == pin_start, "pin and ready range read allocate nothing");
  }

  check(Counted::copies == 0, "ten ticks copy no domain rows");

  return {allocation_test::count - before, allocation_test::bytes - bytes};
}

void stableIdsAndScale() {
  auto original = std::make_shared<Page>(1, RowRange{0, 2}, 10);
  auto reordered = std::make_shared<Page>(2, RowRange{0, 2}, 10);

  std::swap(reordered->values[0], reordered->values[1]);

  auto inserted = std::make_shared<Page>(3, RowRange{1, 2}, 10);
  Fixture f;

  take(f.source->publisher().publish(original));
  take(f.scheduler->dispatch());

  auto identity = take(take(f.source->pin()).read({0, 2})).rows()[0].id;

  take(f.source->publisher().publish(reordered));
  take(f.scheduler->dispatch());
  check(take(take(f.source->pin()).read({0, 2})).rows()[1].id == identity,
        "reordering preserves selected row identity");
  take(f.source->publisher().publish(inserted));
  take(f.scheduler->dispatch());
  check(take(take(f.source->pin()).read({1, 2})).rows()[0].id == identity, "insertion changes position not identity");
  check(RowId{42, 2} != identity && ColumnId{7, 1} != ColumnId{7, 2},
        "generation distinguishes reused row and column values");

  auto small = publicationCost(100);
  auto large = publicationCost(1000000);

  check(small == large, "million logical rows have exactly the same publication allocations/bytes as 100 rows");
}

void factoryAllocationFailures() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto initial = std::make_shared<const Page>(1, RowRange{0, 2}, 10);
  bool created{};
  constexpr std::ptrdiff_t PROBE_LIMIT = 64;

  for (std::ptrdiff_t budget = 0; budget != PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    auto result = Model<int>::create(*tree, root, initial);

    allocation_test::remaining = -1;

    if (result) {
      created = true;

      break;
    }

    check(result.error() == TreeError::NO_MEMORY, "factory allocation failure remains observable");
  }

  check(created, "factory eventually succeeds after finite allocations");

  auto model = take(Model<int>::create(*tree, root, initial, {2, 1, 1}));
  bool opened{};

  for (std::ptrdiff_t budget = 0; budget != PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    auto result = model->openRange();

    allocation_test::remaining = -1;

    if (result) {
      opened = true;

      break;
    }

    check(result.error() == TreeError::NO_MEMORY, "open allocation failure does not leak consumer capacity");
  }

  check(opened, "consumer opens after allocation failures");

  auto consumer = take(model->openRange());
  bool requested{};

  for (std::ptrdiff_t budget = 0; budget != PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    auto result = consumer.request({0, 2});

    allocation_test::remaining = -1;

    if (result) {
      requested = true;

      break;
    }

    check(result.error() == TreeError::NO_MEMORY, "request allocation failure does not leak work capacity");
  }

  check(requested, "request succeeds after allocation failures");
}

void sourceDestructionReentry() {
  struct ReleasingPage : Page {
    std::function<void()> action;

    ReleasingPage() : Page(1, RowRange{0, 2}, 10) {
    }

    ~ReleasingPage() override {
      action();
    }
  };

  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  std::unique_ptr<Model<int>> source;

  auto page = std::make_shared<ReleasingPage>();
  bool released{};

  page->action = [&] {
    released = true;
    source.reset();
  };
  source = take(Model<int>::create(*tree, root, page));
  page.reset();

  auto publisher = source->publisher();

  take(publisher.publish(std::make_shared<const Page>(2, RowRange{0, 2}, 20)));
  take(scheduler->dispatch());
  check(released && !source, "old snapshot destructor can close source after publication commit");

  auto late = publisher.publish(std::make_shared<const Page>(3, RowRange{0, 2}, 30));

  check(!late && late.error() == SchedulerError::CLOSED, "destructor reentry closes the worker endpoint");
}

void rejectedCompletionDestructorReentry() {
  SchedulerOptions options;

  options.data_capacity = 1;

  auto scheduler = take(Scheduler::create(options));
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto model = take(Model<int>::create(*tree, root, std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  auto consumer = take(model->openRange());
  auto request = std::make_unique<ModelRequest<int>>(take(consumer.request({0, 2})));

  struct ReleasingPage : Page {
    std::function<void()> action;

    ReleasingPage(std::function<void()> release) : Page(1, RowRange{0, 2}, 10), action(std::move(release)) {
    }

    ~ReleasingPage() override {
      action();
    }
  };

  take(model->publisher().publish(std::make_shared<const Page>(2, RowRange{0, 2}, 20)));

  auto result = request->complete(std::make_shared<const ReleasingPage>([&] {
    request.reset();
  }));

  check(!request && !result && result.error() == SchedulerError::WOULD_BLOCK,
        "rejected snapshot destructor may destroy the invoking request token");
}

void assignmentDestructorReentry() {
  struct ReleasingPage : Page {
    std::function<void()> action;

    ReleasingPage() : Page(1, RowRange{0, 2}, 10) {
    }

    ~ReleasingPage() override {
      if (action) {
        action();
      }
    }
  };

  for (bool move : {false, true}) {
    Fixture f;
    auto consumer = take(f.source->openRange());
    std::unique_ptr<ModelRequest<int>> request;
    auto storage = std::make_shared<ReleaseAction>();

    storage->action = [&] {
      request.reset();
    };
    request = std::make_unique<ModelRequest<int>>(take(consumer.request({0, 2}, storage)));
    storage.reset();

    auto replacement = take(consumer.request({2, 2}));

    if (move) {
      *request = std::move(replacement);
    } else {
      *request = replacement;
    }

    check(!request, "old work owner may destroy the assigned request after all fields are updated");

    std::unique_ptr<ModelRange<int>> range;
    auto snapshot = std::make_shared<ReleasingPage>();

    snapshot->action = [&] {
      range.reset();
    };
    range = std::make_unique<ModelRange<int>>(take(ModelRange<int>::read(snapshot, {0, 2}, 2)));
    snapshot.reset();

    auto replacement_range = take(take(f.source->pin()).read({0, 2}));

    if (move) {
      *range = std::move(replacement_range);
    } else {
      *range = replacement_range;
    }

    check(!range, "old snapshot may destroy the assigned range after metadata is updated");

    std::unique_ptr<ModelFrame<int>> frame;
    auto frame_snapshot = std::make_shared<ReleasingPage>();

    frame_snapshot->action = [&] {
      frame.reset();
    };

    auto model = take(Model<int>::create(*f.tree, f.root, frame_snapshot));

    frame = std::make_unique<ModelFrame<int>>(take(model->pin()));
    frame_snapshot.reset();
    model.reset();

    auto replacement_frame = take(f.source->pin());

    if (move) {
      *frame = std::move(replacement_frame);
    } else {
      *frame = replacement_frame;
    }

    check(!frame, "old snapshot may destroy the assigned frame after all fields are updated");
  }

  Fixture f;
  std::unique_ptr<ModelConsumer<int>> consumer;
  auto snapshot = std::make_shared<ReleasingPage>();

  snapshot->action = [&] {
    consumer.reset();
  };

  auto model = take(Model<int>::create(*f.tree, f.root, snapshot));

  consumer = std::make_unique<ModelConsumer<int>>(take(model->openRange()));
  snapshot.reset();
  model.reset();

  auto replacement = take(f.source->openRange());

  *consumer = std::move(replacement);
  check(!consumer, "retired state may destroy the assigned consumer after replacement is complete");

  auto remaining = take(f.source->openRange());

  check(remaining.id().value != 0, "reentrant assignment retires the replacement slot exactly once");
}

void allocationFailure() {
  Fixture f;
  auto consumer = take(f.source->openRange());
  auto before = take(f.source->pin()).revision();

  allocation_test::remaining = 0;

  auto rejected = consumer.request({0, 2});

  allocation_test::remaining = -1;
  check(!rejected && rejected.error() == TreeError::NO_MEMORY && take(f.source->pin()).revision() == before,
        "allocation failure leaves request state unchanged");

  auto request = take(consumer.request({0, 2}));

  take(request.complete(std::make_shared<const Page>(1, RowRange{0, 2}, 10)));
  allocation_test::remaining = 0;
  take(f.scheduler->dispatch());
  allocation_test::remaining = -1;
  check(take(f.source->lastError()) == TreeError::NO_MEMORY,
        "UI commit allocation failure is observable after queue acceptance");
  check(take(take(f.source->pin()).range(consumer.id())).state() == RangeState::PENDING,
        "failed completion leaves prior state");
}
} // namespace

int main() {
  independentRangesAndFrame();
  retainedRangeIsSoleStorageOwner();
  equalOverlappingAndGenerations();
  revisionErrorAndLifetime();
  boundedWorkAndReentry();
  mailboxBackpressureAndAffinity();
  stableIdsAndScale();
  allocationFailure();
  factoryAllocationFailures();
  sourceDestructionReentry();
  rejectedCompletionDestructorReentry();
  assignmentDestructorReentry();

  return failures ? 1 : 0;
}
