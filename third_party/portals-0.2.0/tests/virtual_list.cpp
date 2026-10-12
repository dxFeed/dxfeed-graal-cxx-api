// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/test_terminal.hpp"
#include <cstdlib>
#include <iostream>
#include <portals/virtual_list.hpp>

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

// Match nothrow allocation used by standard-library temporary buffers to this fixture's allocator.
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

void check(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    std::cerr << "fixture failed\n";
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

struct Page : ModelSnapshot<int> {
  std::vector<ModelRow<int>> rows;
  RowRange available;
  RangeState state;
  mutable std::size_t calls{};
  mutable std::size_t requested{};

  Page(std::uint64_t revision, std::uint64_t count, RowRange range, RangeState ready = RangeState::READY)
      : ModelSnapshot(revision, count), available(range), state(ready) {
    for (std::size_t i = 0; i < range.count; ++i) {
      rows.push_back({{range.first + i + 1, 1}, static_cast<int>(i)});
    }
  }

  ModelRows<int> read(RowRange range) const override {
    ++calls;
    requested += range.count;

    if (state == RangeState::FAILED) {
      return {state, {}, "failed"};
    }

    if (state == RangeState::READY && range.first >= available.first &&
        range.first - available.first <= available.count &&
        range.count <= available.count - (range.first - available.first)) {
      return {state, std::span(rows).subspan(static_cast<std::size_t>(range.first - available.first), range.count), {}};
    }

    return {};
  }
};

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
};

void boundedDelegates() {
  for (const auto count : {100ULL, 1000000ULL}) {
    Fixture f;
    auto page = std::make_shared<Page>(1, count, RowRange{8, 7});
    auto model = take(Model<int>::create(*f.tree, f.root, page));
    int formatted{};
    auto node = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), [&](const ModelRow<int> &) {
      ++formatted;

      return take(GraphemeText::from("row"));
    }));

    take(setVirtualListFirst(*f.tree, node, 10));

    auto request = take(prepareVirtualList(*f.tree, node, take(model->pin()), {8, 3}));

    check(!request && formatted == 3 && page->requested == 7, "viewport plus overscan, no invisible formatting");
    check(take(f.tree->inspect(node)).children.empty(), "no row widgets");
    take(f.tree->read(node, [&](const VirtualList<int> &list) {
      check(list.visibleRange() == RowRange{10, 3}, "visible logical interval");
      check(list.preparedRange() == RowRange{8, 7}, "bounded prepared interval");
    }));
  }
}

struct SurfaceSink : PaintContext {
  NodeGeometry geometry;
  std::vector<RenderCommand> &commands;

  SurfaceSink(NodeGeometry value, std::vector<RenderCommand> &output) : geometry(value), commands(output) {
  }

  std::expected<void, TreeError> submit(std::span<const RenderCommand> values) override {
    for (const auto &value : values) {
      std::visit(
        [&](const auto &command) {
          auto result = command;

          result.clip = take(command.clip.translated(geometry.surface.origin())).intersection(geometry.clip);

          if constexpr (std::is_same_v<std::decay_t<decltype(command)>, TextCommand>) {
            result.origin = take(translated(command.origin, geometry.surface.origin()));
          } else {
            result.area = take(command.area.translated(geometry.surface.origin()));
          }

          commands.emplace_back(result);
        },
        value);
    }

    return {};
  }
};

struct Screen {
  test_support::FakeClock clock;
  test_support::TestTerminal terminal{clock, {8, 3}};
  FrameWriter writer{terminal};

  std::expected<void, TreeError> draw(Fixture &f, Node<VirtualList<int>> node, CellSize size, CellSize clip_size = {},
                                      CellPoint origin = {}) {
    terminal.resize(size);

    const auto viewport = take(CellRect::from({}, clip_size == CellSize{} ? size : clip_size));
    const auto bounds = take(CellRect::from(origin, size));
    std::optional<CellBuffer> buffer;
    std::expected<void, TreeError> result;

    take(f.tree->invalidate(f.root, Invalidation::LAYOUT));
    take(f.tree->frame([&] {
      take(layoutTree(*f.tree, f.root, bounds, viewport));

      std::vector<RenderCommand> commands;
      SurfaceSink sink(take(f.tree->inspect(node)).geometry, commands);

      result = f.tree->paint(node, sink);

      if (result) {
        buffer.emplace(take(renderFullFrame(size, commands)));
      }
    }));

    if (buffer) {
      take(writer.submit(std::move(*buffer)));

      const auto written = writer.flush(TerminalDeadline::max());

      check(!written.error && !writer.pending(), "writer finishes independently observed output");
    }

    return result;
  }

  std::string row(int y) const {
    const auto snapshot = terminal.snapshot();
    std::string result;

    for (int x = 0; x < snapshot.size.width; ++x) {
      result += snapshot.cells[static_cast<std::size_t>(y * snapshot.size.width + x)].glyph;
    }

    return result;
  }
};

auto label() {
  return [](const ModelRow<int> &row) {
    return take(GraphemeText::from(std::to_string(row.id.value)));
  };
}

std::optional<RowId> selected(Fixture &f, Node<VirtualList<int>> node) {
  std::optional<RowId> value;

  take(f.tree->read(node, [&](const auto &list) {
    value = list.selection();
  }));

  return value;
}

void renderGeometryAndInput() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  VirtualListOptions options;

  options.row_height = 2;
  options.selected.background = RgbColor{20, 40, 60};

  auto list = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label(), options));
  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualList(*router, *f.tree, list));
  Screen screen;

  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 5}));
  take(screen.draw(f, list, {4, 5}));
  check(screen.row(0) == "1   " && screen.row(1) == "    " && screen.row(2) == "2   " && screen.row(4) == "3   ",
        "fixed row height includes final partial row");

  take(screen.draw(f, list, {4, 5}, {}, {1, 1}));
  check(screen.row(0) == "    " && screen.row(1) == " 1  ", "widget-local commands translate through real geometry");
  take(router->dispatch(InputEvent{MouseInputEvent{1, 3, MouseButton::LEFT, MouseAction::PRESS}}));
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 5}));
  check(selected(f, list) == RowId{2, 1}, "mouse subtracts surface origin before fixed-row lookup");

  const auto reads = page->calls;

  take(screen.draw(f, list, {4, 5}));
  check(page->calls == reads, "layout and paint never read or format model");

  auto clicked = take(router->dispatch(InputEvent{MouseInputEvent{1, 3, MouseButton::LEFT, MouseAction::PRESS}}));

  check(clicked.status == DispatchStatus::HANDLED, "real geometry routes click to row");
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 5}));
  check(selected(f, list) == RowId{2, 1}, "click in second cell row selects stable ID");
  take(screen.draw(f, list, {4, 5}));
  check(screen.terminal.snapshot().cells[8].style.background == options.selected.background,
        "selected row style is rendered");
  take(router->invoke(list, {Action::MOVE_DOWN}));
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 5}));
  check(selected(f, list) == RowId{3, 1}, "down selects next stable row");
  take(screen.draw(f, list, {4, 5}));
  take(router->dispatch(InputEvent{MouseInputEvent{1, 1, MouseButton::WHEEL_DOWN, MouseAction::SCROLL}}));
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 5}));
  take(screen.draw(f, list, {4, 5}));
  check(screen.row(0) == "2   " && selected(f, list) == RowId{3, 1},
        "wheel changes first row without changing selection");

  const auto stale = screen.draw(f, list, {3, 4});

  check(!stale && stale.error() == TreeError::INVALID_GEOMETRY, "resize requires explicit preparation");
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {3, 4}));
  take(screen.draw(f, list, {3, 4}, {2, 2}));
  check(screen.row(0) == "2  " && screen.row(2) == "   ", "ancestor viewport clips real rendered rows");
  take(f.tree->frame([&] {
    auto rejected = prepareVirtualList(*f.tree, list, take(model->pin()), {3, 4});

    check(!rejected && rejected.error() == TreeError::WOULD_BLOCK, "preparation forbidden during frame");
  }));
}

void unicodeEmptyAndLimits() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  auto list = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), [](const auto &) {
    return take(GraphemeText::from("\xE7\x95\x8C"
                                   "a"));
  }));

  Screen screen;

  take(prepareVirtualList(*f.tree, list, take(model->pin()), {1, 2}));
  take(screen.draw(f, list, {1, 2}));
  check(screen.row(0) == " ", "wide grapheme cannot leave an orphan in one cell");
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {2, 2}));
  take(screen.draw(f, list, {2, 2}));
  check(screen.terminal.snapshot().cells[0].glyph == "\xE7\x95\x8C" && screen.terminal.snapshot().cells[1].tail,
        "wide glyph keeps continuation");

  VirtualListOptions bad;

  bad.row_height = 0;

  auto rejected = setVirtualListOptions(*f.tree, list, bad);

  check(!rejected && rejected.error() == TreeError::INVALID_ARGUMENT, "zero row height explicit");
  bad.row_height = 1;
  bad.overscan_rows = std::numeric_limits<std::size_t>::max();
  take(setVirtualListOptions(*f.tree, list, bad));

  auto huge = std::make_shared<Page>(2, std::numeric_limits<std::uint64_t>::max(), RowRange{});

  take(model->publisher().publish(huge));
  take(f.scheduler->dispatch());
  take(setVirtualListFirst(*f.tree, list, std::numeric_limits<std::uint64_t>::max()));

  auto too_large = prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3});

  check(!too_large && too_large.error() == TreeError::INVALID_ARGUMENT, "oversized range never wraps or truncates");
  bad.overscan_rows = 2;
  take(setVirtualListOptions(*f.tree, list, bad));

  auto last = take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3}));

  check(last && last->range() == RowRange{std::numeric_limits<std::uint64_t>::max() - 3, 3},
        "last row and overscan checked at uint64 maximum");

  auto empty = std::make_shared<Page>(3, 0, RowRange{});

  take(model->publisher().publish(empty));
  take(f.scheduler->dispatch());
  check(!take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3})), "empty model schedules no work");
  take(screen.draw(f, list, {4, 3}));
  check(screen.row(0) == "    ", "empty model draws blank frame");

  const auto empty_reads = empty->calls;

  take(prepareVirtualList(*f.tree, list, take(model->pin()), {0, 3}));
  take(f.tree->consumeFrameRequest());
  take(prepareVirtualList(*f.tree, list, take(model->pin()), {0, 3}));
  check(empty->calls == empty_reads && take(f.tree->consumeFrameRequest()) == Invalidation::NONE,
        "empty or zero-width preparation does no provider work and stays idle");
}

void pendingAndIdentity() {
  Fixture f;
  auto pending = std::make_shared<Page>(1, 100, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, pending));
  auto a = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label()));
  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualList(*router, *f.tree, a));
  auto pin = take(model->pin());
  auto old = take(prepareVirtualList(*f.tree, a, pin, {4, 3}));

  check(old.has_value(), "pending returns one explicit dispatch receipt");
  check(!take(prepareVirtualList(*f.tree, a, pin, {4, 3})), "same pin never redispatches pending work");

  Screen screen;

  take(screen.draw(f, a, {4, 3}));
  check(screen.row(0) == "... ", "pending placeholder renders through writer");
  take(router->invoke(a, {Action::MOVE_END}));

  auto latest = take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));

  check(latest && latest->range() == RowRange{95, 5} && old->stopRequested(), "pending End scroll replaces request");

  auto late = old->complete(std::make_shared<Page>(1, 100, old->range()));

  check(!late && late.error() == SchedulerError::CLOSED, "old completion cannot alter selection");
  take(latest->complete(std::make_shared<Page>(1, 100, latest->range())));
  take(f.scheduler->dispatch());

  auto complete_pin = take(model->pin());

  take(prepareVirtualList(*f.tree, a, complete_pin, {4, 3}));
  check(selected(f, a) == RowId{100, 1}, "latest pending navigation selects its resulting ID");
  take(screen.draw(f, a, {4, 3}));
  check(screen.row(2) == "100 ", "only latest viewport data is painted");

  auto revised = std::make_shared<Page>(2, 101, RowRange{0, 101});

  revised->rows[0].id = {100, 1};
  revised->rows[99].id = {999, 1};
  take(model->publisher().publish(revised));
  take(f.scheduler->dispatch());
  take(setVirtualListFirst(*f.tree, a, 0));
  take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));
  check(selected(f, a) == RowId{100, 1}, "insertion and reorder preserve stable selection");
  take(router->invoke(a, {Action::MOVE_DOWN}));
  take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));
  check(selected(f, a) == RowId{2, 1}, "navigation locates selected identity in the bounded visible range");
}

void consumersHideRetryAndSelectionIntent() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  auto a = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label()));
  auto b = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label()));

  take(setVirtualListFirst(*f.tree, b, 20));

  auto pin = take(model->pin());
  auto ar = take(prepareVirtualList(*f.tree, a, pin, {4, 3}));
  auto br = take(prepareVirtualList(*f.tree, b, pin, {4, 3}));

  take(br->complete(std::make_shared<Page>(1, 100, br->range())));
  take(f.scheduler->dispatch());

  auto next = take(model->pin());

  take(prepareVirtualList(*f.tree, a, next, {4, 3}));
  take(prepareVirtualList(*f.tree, b, next, {4, 3}));
  take(f.tree->read(a, [](const auto &view) {
    check(view.rangeState() == RangeState::PENDING, "other consumer remains pending");
  }));
  take(f.tree->read(b, [](const auto &view) {
    check(view.rangeState() == RangeState::READY, "completion updates its consumer");
  }));

  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualList(*router, *f.tree, a));

  take(router->invoke(a, {Action::MOVE_DOWN}));
  take(setVirtualListSelection(*f.tree, a, RowId{77, 1}));
  take(ar->complete(std::make_shared<Page>(1, 100, ar->range())));
  take(f.scheduler->dispatch());
  take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));
  check(selected(f, a) == RowId{77, 1}, "explicit selection cancels pending navigation intent");
  take(setVirtualListFirst(*f.tree, a, 40));

  auto hidden_work = take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));

  take(f.tree->setVisible(f.root, false));
  take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));
  check(hidden_work->stopRequested(), "hidden ancestor cancels pending work");
  take(f.tree->read(a, [](const auto &view) {
    check(view.visibleRange().count == 0, "hidden view releases delegates");
  }));
  take(f.tree->setVisible(f.root, true));

  auto retry = take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));

  take(retry->complete(std::make_shared<Page>(1, 100, retry->range(), RangeState::FAILED)));
  take(f.scheduler->dispatch());
  take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));
  take(f.tree->read(a, [](const auto &view) {
    check(view.rangeState() == RangeState::FAILED, "provider error is observable");
  }));
  take(f.tree->remove(b));

  Screen screen;

  take(screen.draw(f, a, {4, 3}));
  check(screen.row(0) == "!   ", "provider error placeholder renders explicitly");
  take(retryVirtualList(*f.tree, a));

  auto retried = take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));

  check(retried.has_value(), "explicit provider retry gives fresh dispatch receipt");
  retried->cancel();

  auto again = take(prepareVirtualList(*f.tree, a, take(model->pin()), {4, 3}));

  check(again.has_value(), "rejected worker dispatch is retryable after cancellation");
  take(f.tree->remove(a));
  check(again->stopRequested(), "removal cancels worker without joining");
}

void transactionalFailuresAndLifetime() {
  Fixture f;
  auto initial = std::make_shared<Page>(1, 100, RowRange{0, 10});
  std::weak_ptr<const Page> lifetime = initial;
  auto model = take(Model<int>::create(*f.tree, f.root, initial));
  bool fail{};
  bool allocation{};
  int calls{};
  auto list = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), [&](const auto &row) {
    ++calls;

    if (fail && calls % 3 == 2) {
      if (allocation) {
        throw std::bad_alloc();
      }

      throw std::runtime_error("delegate failed");
    }

    return take(GraphemeText::from(std::to_string(row.id.value)));
  }));

  take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3}));
  initial.reset();

  auto replacement = std::make_shared<Page>(2, 100, RowRange{0, 10});

  replacement->rows[0].id = {55, 1};
  take(model->publisher().publish(replacement));
  take(f.scheduler->dispatch());

  auto pin = take(model->pin());

  fail = true;

  Screen screen;

  for (bool bad_alloc : {false, true}) {
    calls = 0;
    allocation = bad_alloc;

    const auto result = prepareVirtualList(*f.tree, list, pin, {4, 3});

    check(!result && result.error() == (bad_alloc ? TreeError::NO_MEMORY : TreeError::CALLBACK_FAILED),
          "delegate/allocation error returns without mixed presentation");
    take(screen.draw(f, list, {4, 3}));
    check(screen.row(0) == "1   " && screen.row(1) == "2   ", "failed prepare retains entire previous frame and text");
    check(!lifetime.expired(), "retained prepared frame owns old provider storage");
  }

  fail = false;
  take(prepareVirtualList(*f.tree, list, pin, {4, 3}));
  check(lifetime.expired(), "successful prepare releases old frame owner");
  take(screen.draw(f, list, {4, 3}));
  check(screen.row(0) == "55  ", "successful retry commits new presentation");

  auto foreign = take(Model<int>::create(*f.tree, f.root, replacement));
  const auto wrong = prepareVirtualList(*f.tree, list, take(foreign->pin()), {4, 3});

  check(!wrong && wrong.error() == TreeError::STALE_NODE,
        "foreign frame is rejected even when cached revision matches");
  model.reset();
  take(screen.draw(f, list, {4, 3}));
  check(screen.row(0) == "55  ", "prepared owning text survives source destruction through real frame");
}

void allocationTransactionAndIdle() {
  Fixture f;
  auto first = std::make_shared<Page>(1, 100, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, first));
  auto list = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label()));
  auto initial_pin = take(model->pin());

  take(prepareVirtualList(*f.tree, list, initial_pin, {4, 3}));
  take(f.tree->consumeFrameRequest());
  take(prepareVirtualList(*f.tree, list, initial_pin, {4, 3}));
  check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE,
        "unchanged preparation does not schedule idle frames");

  auto second = std::make_shared<Page>(2, 100, RowRange{0, 10});

  second->rows[0].id = {22, 1};
  take(model->publisher().publish(second));
  take(f.scheduler->dispatch());

  auto pin = take(model->pin());
  Screen screen;
  bool completed{};
  int rejected{};

  for (std::ptrdiff_t budget = 0; budget < 256 && !completed; ++budget) {
    allocation_test::remaining = budget;

    auto result = prepareVirtualList(*f.tree, list, pin, {4, 3});

    allocation_test::remaining = -1;

    if (result) {
      completed = true;
    } else {
      ++rejected;
      check(result.error() == TreeError::NO_MEMORY, "real allocation failure is explicit no_memory");
      take(screen.draw(f, list, {4, 3}));
      check(screen.row(0) == "1   ", "real allocation failure preserves previous complete presentation");
    }
  }

  check(completed && rejected > 0, "allocation failpoints eventually commit");
  take(screen.draw(f, list, {4, 3}));
  check(screen.row(0) == "22  ", "successful allocation retry publishes new text");
}

void prefetchedCompletionNavigation() {
  Fixture f;
  auto base = std::make_shared<Page>(1, 1000, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, base, ModelOptions{9, 2, 4}));
  auto consumer = take(model->openRange());
  const auto id = consumer.id();
  auto view = take(f.tree->emplace<VirtualList<int>>(f.root, std::move(consumer), label()));

  take(setVirtualListFirst(*f.tree, view, 100));

  auto work = take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));

  check(work && work->range() == RowRange{98, 7}, "request remains exact and within the model budget");

  auto cached = std::make_shared<Page>(1, 1000, RowRange{80, 60});
  const std::weak_ptr<Page> lifetime = cached;

  take(work->complete(cached));
  take(f.scheduler->dispatch());
  work.reset();
  cached.reset();
  take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));

  for (std::uint64_t first = 101; first <= 120; ++first) {
    take(setVirtualListFirst(*f.tree, view, first));
    check(!take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3})),
          "forward navigation reuses the bounded completion without another request");
  }

  for (std::uint64_t first = 120; first >= 90; --first) {
    take(setVirtualListFirst(*f.tree, view, first));
    check(!take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3})),
          "backward navigation reuses the same immutable completion");
  }

  {
    const auto pin = take(model->pin());

    check(take(pin.range(id)).range() == RowRange{98, 7}, "consumer result stays exact, without spatial merging");
    check(take(pin.read({88, 7})).state() == RangeState::PENDING, "public frame read still uses only the base");
  }

  take(f.tree->read(view, [](const auto &value) {
    check(value.rangeState() == RangeState::READY && value.preparedRange() == RowRange{88, 7},
          "ready presentation owns precisely the current requested interval");
  }));

  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualList(*router, *f.tree, view));

  take(setVirtualListSelection(*f.tree, view, RowId{91, 1}));
  take(router->invoke(view, {Action::MOVE_DOWN}));
  take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));
  check(selected(f, view) == RowId{92, 1}, "cached navigation selects the current row's full identity");

  Screen screen;

  take(screen.draw(f, view, {4, 3}));
  check(screen.row(0) == "91  ", "cached rows are formatted at their actual current logical indices");

  auto oversized = prepareVirtualList(*f.tree, view, take(model->pin()), {8, 10});

  check(!oversized && oversized.error() == TreeError::INVALID_ARGUMENT,
        "prefetched storage cannot bypass the synchronous row budget");
  take(setVirtualListFirst(*f.tree, view, 200));

  auto miss = take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));

  check(miss.has_value(), "cache miss requests the new exact viewport");
  take(screen.draw(f, view, {4, 3}));
  check(screen.row(0) == "... ", "cache miss displays pending, not rows from the old viewport");
  take(miss->complete(std::make_shared<Page>(1, 1000, RowRange{190, 60})));
  take(f.scheduler->dispatch());
  take(setVirtualListFirst(*f.tree, view, 90));
  check(!take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3})) && miss->stopRequested(),
        "reverse after a delayed completion reuses the ready retained page");
  miss.reset();

  take(model->publisher().publish(std::make_shared<Page>(2, 1001, RowRange{}, RangeState::PENDING)));
  take(f.scheduler->dispatch());

  auto revised = take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));

  check(revised && revised->dataRevision() == 2, "new revision requests its own rows, not cached old data");
  check(lifetime.expired(), "old completion storage is released after revision and presentation replacement");
  check(selected(f, view) == RowId{92, 1}, "pending revision does not assign identity from a stale ordinal");
  take(screen.draw(f, view, {4, 3}));
  check(screen.row(0) == "... ", "old revision rows are not displayed while the new revision is pending");

  auto wrongCount = revised->complete(std::make_shared<Page>(2, 1000, RowRange{80, 60}));

  check(!wrongCount && wrongCount.error() == SchedulerError::INVALID_ARGUMENT,
        "completion row count must agree with the requesting domain snapshot");
  take(revised->complete(std::make_shared<Page>(2, 1001, RowRange{80, 60})));
  take(f.scheduler->dispatch());
  revised.reset();
  take(prepareVirtualList(*f.tree, view, take(model->pin()), {4, 3}));
  check(!take(prepareVirtualList(*f.tree, view, take(model->pin()), {0, 0})),
        "zero viewport does not request or materialize rows");
}

void singleBudgetRetryAndAffinity() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, page, ModelOptions{16, 1, 1}));
  auto list = take(f.tree->emplace<VirtualList<int>>(f.root, take(model->openRange()), label()));
  auto first = take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3}));

  take(setVirtualListFirst(*f.tree, list, 10));

  auto full = prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3});

  check(!full && full.error() == TreeError::WOULD_BLOCK, "cancelled caller token retains finite work budget");
  first.reset();

  auto next = take(prepareVirtualList(*f.tree, list, take(model->pin()), {4, 3}));

  check(next && next->range() == RowRange{8, 7}, "releasing cancelled receipt restores single-slot progress");

  auto pin = take(model->pin());
  std::expected<std::optional<ModelRequest<int>>, TreeError> foreign;
  std::thread worker([&] {
    foreign = prepareVirtualList(*f.tree, list, pin, {4, 3});
  });

  worker.join();
  check(!foreign && foreign.error() == TreeError::WRONG_THREAD,
        "preparation rejects foreign thread before provider read");
}
} // namespace

int main() {
  prefetchedCompletionNavigation();
  singleBudgetRetryAndAffinity();
  allocationTransactionAndIdle();
  boundedDelegates();
  renderGeometryAndInput();
  unicodeEmptyAndLimits();
  pendingAndIdentity();
  consumersHideRetryAndSelectionIntent();
  transactionalFailuresAndLifetime();

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
