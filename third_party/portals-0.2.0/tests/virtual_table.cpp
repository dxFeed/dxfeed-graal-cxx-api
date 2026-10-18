// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/test_terminal.hpp"
#include <array>
#include <cstdlib>
#include <format>
#include <iostream>
#include <portals/virtual_table.hpp>

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

void boundedColumns() {
  for (const auto count : {100ULL, 1000000ULL}) {
    Fixture f;
    auto page = std::make_shared<Page>(1, count, RowRange{8, 7});
    auto model = take(Model<int>::create(*f.tree, f.root, page));
    int first_calls{};
    int second_calls{};
    int hidden_calls{};
    std::vector<TableColumn<int>> columns;

    columns.push_back({{1, 1}, take(GraphemeText::from("A")), 4, [&](const int &value) {
                         ++first_calls;
                         return take(GraphemeText::from(std::to_string(value)));
                       }});
    columns.push_back({{2, 1}, take(GraphemeText::from("B")), 4, [&](const int &value) {
                         ++second_calls;
                         return take(GraphemeText::from(std::to_string(value)));
                       }});
    columns.push_back({{3, 1}, take(GraphemeText::from("C")), 4, [&](const int &) {
                         ++hidden_calls;
                         return take(GraphemeText::from("hidden"));
                       }});

    auto node = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), std::move(columns)));

    take(setVirtualTableFirst(*f.tree, node, 10));
    check(!take(prepareVirtualTable(*f.tree, node, take(model->pin()), {8, 4})), "ready table needs no request");
    check(first_calls == 3 && second_calls == 3 && hidden_calls == 0, "only visible rows and columns format");
    check(page->calls == 1 && page->requested == 7, "all columns share exactly one bounded snapshot read");
    check(take(f.tree->inspect(node)).children.empty(), "no cell or row widgets");
    auto remote = take(model->openRange());
    auto update = take(remote.request({count - 1, 1}));

    take(update.complete(std::make_shared<Page>(1, count, RowRange{count - 1, 1})));
    take(f.scheduler->dispatch());
    take(prepareVirtualTable(*f.tree, node, take(model->pin()), {8, 4}));
    check(first_calls <= 6 && second_calls <= 6 && hidden_calls == 0 && page->requested <= 14,
          "outside-viewport update remains bounded independently of logical count");

    take(f.tree->read(node, [](const auto &table) {
      check(table.visibleRange() == RowRange{10, 3}, "header consumes one viewport row");
      check(table.preparedRange() == RowRange{8, 7}, "overscan stays unformatted");
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

  template <class T>
  std::expected<void, TreeError> draw(Fixture &f, Node<VirtualTable<T>> node, CellSize size, CellSize clip_size = {},
                                      CellPoint origin = {}, std::function<void()> during_frame = {}) {
    terminal.resize(size);

    const auto viewport = take(CellRect::from({}, clip_size == CellSize{} ? size : clip_size));
    const auto bounds = take(CellRect::from(origin, size));
    std::optional<CellBuffer> buffer;
    std::expected<void, TreeError> result;

    take(f.tree->invalidate(f.root, Invalidation::LAYOUT));
    take(f.tree->frame([&] {
      take(layoutTree(*f.tree, f.root, bounds, viewport));

      if (during_frame) {
        during_frame();
      }

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

std::optional<RowId> selected(Fixture &f, Node<VirtualTable<int>> node) {
  std::optional<RowId> value;

  take(f.tree->read(node, [&](const auto &list) {
    value = list.selection();
  }));

  return value;
}

GraphemeText numberText(int value) {
  constexpr auto INTEGER_TEXT_CAPACITY = std::numeric_limits<int>::digits10 + 3;
  std::array<char, INTEGER_TEXT_CAPACITY> buffer{};
  const auto end = std::format_to(buffer.begin(), "{}", value);

  return take(GraphemeText::from({buffer.data(), static_cast<std::size_t>(end - buffer.begin())}));
}

std::vector<TableColumn<int>> columns(std::int32_t width = 4) {
  return {{{1, 1},
           take(GraphemeText::from("A")),
           width,
           [](const int &value) {
             return numberText(value);
           }},
          {{2, 1}, take(GraphemeText::from("B")), width, [](const int &value) {
             return numberText(value + 10);
           }}};
}

void geometryAndSelection() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  VirtualTableOptions options;

  options.row_height = 2;
  options.selected.background = RgbColor{20, 40, 60};
  options.header.background = RgbColor{5, 10, 15};

  auto table = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns(), options));
  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualTable(*router, *f.tree, table));
  Screen screen;

  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  take(screen.draw(f, table, {8, 6}));
  check(screen.row(0) == "A   B   " && screen.row(1) == "0   10  " && screen.row(2) == "        " &&
          screen.row(5) == "2   12  ",
        "header and fixed-height partial final row render");
  take(router->dispatch(InputEvent{MouseInputEvent{1, 0, MouseButton::LEFT, MouseAction::PRESS}}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  check(!selected(f, table), "header click cannot select a data row");
  take(router->dispatch(InputEvent{MouseInputEvent{5, 4, MouseButton::LEFT, MouseAction::PRESS}}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  check(selected(f, table) == RowId{2, 1}, "click in second column selects stable row");
  take(screen.draw(f, table, {8, 6}));
  check(screen.terminal.snapshot().cells[3 * 8 + 7].style.background == options.selected.background,
        "selection spans whole row");
  check(screen.terminal.snapshot().cells[0].style.background == options.header.background, "header style independent");

  const auto reads = page->calls;

  take(screen.draw(f, table, {8, 6}, {}, {1, 1}));
  check(page->calls == reads, "layout and paint never consult provider");
  take(router->dispatch(InputEvent{MouseInputEvent{1, 2, MouseButton::LEFT, MouseAction::PRESS}}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  check(selected(f, table) == RowId{1, 1}, "mouse subtracts surface origin and header");
  take(router->invoke(table, {Action::MOVE_DOWN}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  check(selected(f, table) == RowId{2, 1}, "keyboard navigation uses stable identity");
  take(router->dispatch(InputEvent{MouseInputEvent{2, 2, MouseButton::WHEEL_DOWN, MouseAction::SCROLL}}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 6}));
  take(screen.draw(f, table, {8, 6}));
  check(screen.row(1) == "1   11  " && selected(f, table) == RowId{2, 1}, "wheel preserves selection");

  const auto mismatch = screen.draw(f, table, {7, 5});

  check(!mismatch && mismatch.error() == TreeError::INVALID_GEOMETRY, "resize requires preparation");
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {7, 5}));
  take(screen.draw(f, table, {7, 5}, {5, 3}));
  check(screen.row(0) == "A   B  " && screen.row(1) == "1   1  " && screen.row(3) == "       ", "ancestor clipping");
  take(f.tree->consumeFrameRequest());
  take(setVirtualTableSelection(*f.tree, table, RowId{90, 1}));
  check(take(f.tree->consumeFrameRequest()) == Invalidation::PAINT, "selection invalidates paint only");
  take(setVirtualTableColumns(*f.tree, table, columns(2)));
  check(take(f.tree->consumeFrameRequest()) == Invalidation::LAYOUT, "column replacement invalidates layout");
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {7, 5}));
  take(screen.draw(f, table, {7, 5}));
  check(screen.row(0) == "A B    ", "new widths used after preparation");
}

void narrowEmptyAndBudgets() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  auto wide = columns(1);

  wide[0].header = take(GraphemeText::from("\xE7\x95\x8C"));
  wide[0].format = [](const int &) {
    return take(GraphemeText::from("\xE7\x95\x8C"));
  };

  auto table = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), std::move(wide)));
  Screen screen;

  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {2, 3}));
  take(screen.draw(f, table, {2, 3}));
  check(screen.row(0) == " B" && screen.row(1) == " 1", "wide glyph cannot cross column boundary");
  take(setVirtualTableColumns(*f.tree, table, {}));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {2, 3}));
  take(screen.draw(f, table, {2, 3}));
  check(screen.row(0) == "  " && screen.row(1) == "  ", "empty columns clear table");
  take(setVirtualTableColumns(*f.tree, table, columns()));

  const auto reads = page->calls;

  for (auto size : {CellSize{0, 3}, CellSize{8, 0}, CellSize{8, 1}}) {
    check(!take(prepareVirtualTable(*f.tree, table, take(model->pin()), size)), "empty body requests no rows");
  }

  check(page->calls == reads, "zero body dimensions avoid provider reads");

  auto empty = std::make_shared<Page>(2, 0, RowRange{});

  take(model->publisher().publish(empty));
  take(f.scheduler->dispatch());
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 3}));
  take(screen.draw(f, table, {8, 3}));
  check(screen.row(0) == "A   B   " && screen.row(1) == "        ", "empty model retains headers");

  auto duplicate = columns();

  duplicate[1].id = duplicate[0].id;
  check(!setVirtualTableColumns(*f.tree, table, std::move(duplicate)), "duplicate ColumnId rejected");
  take(screen.draw(f, table, {8, 3}));
  check(screen.row(0) == "A   B   ", "rejected columns preserve prepared header");

  VirtualTableOptions options;

  options.max_columns = 1;
  check(!setVirtualTableOptions(*f.tree, table, options), "column budget enforced");
  options.max_columns = 64;
  options.row_height = 0;
  check(!setVirtualTableOptions(*f.tree, table, options), "zero row height rejected");
  options.row_height = 1;
  options.max_prepared_text_bytes = 1;
  check(!setVirtualTableOptions(*f.tree, table, options), "header byte budget enforced");
  options.max_prepared_text_bytes = 8;
  take(setVirtualTableOptions(*f.tree, table, options));

  auto ready = std::make_shared<Page>(3, 100, RowRange{0, 10});

  take(model->publisher().publish(ready));
  take(f.scheduler->dispatch());
  check(!prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}), "aggregate text budget enforced");
  options.max_prepared_text_bytes = 1024;
  options.max_prepared_cells = 3;
  take(setVirtualTableOptions(*f.tree, table, options));
  check(!prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}), "row-column product budget enforced");
  options.max_prepared_cells = 16384;
  options.overscan_rows = std::numeric_limits<std::size_t>::max();
  take(setVirtualTableOptions(*f.tree, table, options));

  auto huge = std::make_shared<Page>(4, std::numeric_limits<std::uint64_t>::max(), RowRange{});

  take(model->publisher().publish(huge));
  take(f.scheduler->dispatch());
  take(setVirtualTableFirst(*f.tree, table, std::numeric_limits<std::uint64_t>::max()));
  check(!prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}), "overscan never wraps");
}

void allocationTransactionAndIdle() {
  Fixture f;
  auto first = std::make_shared<Page>(1, 100, RowRange{0, 10});
  std::weak_ptr<const Page> lifetime = first;
  auto model = take(Model<int>::create(*f.tree, f.root, first));
  bool fail{};
  auto definitions = columns();

  definitions[1].format = [&](const int &value) {
    if (fail) {
      throw std::runtime_error("second column failure");
    }
    return numberText(value + 10);
  };

  auto table = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), std::move(definitions)));
  auto pin = take(model->pin());
  Screen screen;

  take(prepareVirtualTable(*f.tree, table, pin, {8, 4}));
  take(f.tree->consumeFrameRequest());
  take(prepareVirtualTable(*f.tree, table, pin, {8, 4}));
  check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE, "unchanged preparation stays idle");
  first.reset();

  auto next = std::make_shared<Page>(2, 100, RowRange{0, 10});

  next->rows[0].value = 55;
  take(model->publisher().publish(next));
  take(f.scheduler->dispatch());
  pin = take(model->pin());
  fail = true;

  const auto rejected = prepareVirtualTable(*f.tree, table, pin, {8, 4});

  check(!rejected && rejected.error() == TreeError::CALLBACK_FAILED, "throwing second formatter explicit");
  take(screen.draw(f, table, {8, 4}));
  check(screen.row(1) == "0   10  " && !lifetime.expired(), "old snapshot and both old cells retained");
  fail = false;

  bool completed{};
  int failures_seen{};

  for (std::ptrdiff_t budget = 0; budget < 512 && !completed; ++budget) {
    allocation_test::remaining = budget;

    auto result = prepareVirtualTable(*f.tree, table, pin, {8, 4});

    allocation_test::remaining = -1;

    if (result) {
      completed = true;
    } else {
      ++failures_seen;
      check(result.error() == TreeError::NO_MEMORY, "real allocation failure is NO_MEMORY");
      take(screen.draw(f, table, {8, 4}));
      check(screen.row(1) == "0   10  ", "allocation failure preserves complete old presentation");
    }
  }

  check(completed && failures_seen > 0 && lifetime.expired(), "failpoints recover and release old owner");
  take(screen.draw(f, table, {8, 4}));
  check(screen.row(1) == "55  65  ", "retry commits both columns");

  auto foreign = take(Model<int>::create(*f.tree, f.root, next));
  const auto wrong = prepareVirtualTable(*f.tree, table, take(foreign->pin()), {8, 4});

  check(!wrong && wrong.error() == TreeError::STALE_NODE, "foreign frame rejected even on cached revision");
  take(f.tree->frame([&] {
    const auto inside = prepareVirtualTable(*f.tree, table, pin, {8, 4});
    check(!inside && inside.error() == TreeError::WOULD_BLOCK, "prepare forbidden in frame");
  }));
  model.reset();
  take(screen.draw(f, table, {8, 4}));
  check(screen.row(1) == "55  65  ", "text survives model destruction");
}

void cachedFailuresAreNotHidden() {
  struct FaultPage : Page {
    bool throws;

    explicit FaultPage(bool throwing) : Page(1, 1000, RowRange{190, 60}), throws(throwing) {
    }

    ModelRows<int> read(RowRange range) const override {
      if (range.first < available.first) {
        if (throws) {
          throw std::runtime_error("completion read failed");
        }

        return {RangeState::FAILED, {}, "completion range failed"};
      }

      return Page::read(range);
    }
  };

  for (const bool throwing : {false, true}) {
    Fixture f;
    auto base = std::make_shared<Page>(1, 1000, RowRange{}, RangeState::PENDING);
    auto model = take(Model<int>::create(*f.tree, f.root, base));
    auto table = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns()));

    take(setVirtualTableFirst(*f.tree, table, 100));

    auto ready = take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}));

    take(ready->complete(std::make_shared<Page>(1, 1000, RowRange{80, 60})));
    take(f.scheduler->dispatch());
    ready.reset();
    take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}));
    take(setVirtualTableFirst(*f.tree, table, 200));

    auto later = take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}));

    take(later->complete(std::make_shared<FaultPage>(throwing)));
    take(f.scheduler->dispatch());
    take(setVirtualTableFirst(*f.tree, table, 90));

    auto result = prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4});

    if (throwing) {
      check(!result && result.error() == TreeError::CALLBACK_FAILED,
            "throwing cached provider is reported rather than hidden by a ready retained owner");
      take(f.tree->read(table, [](const auto &value) {
        check(value.visibleRange().first == 200 && value.rangeState() == RangeState::PENDING,
              "cached read exception leaves the prior successful presentation intact");
      }));
    } else {
      check(result && !result->has_value(), "failed cached range does not schedule replacement work");
      take(f.tree->read(table, [](const auto &value) {
        check(value.visibleRange().first == 90 && value.rangeState() == RangeState::FAILED,
              "failed cached read is not converted to older ready rows");
      }));
    }
  }
}

void prefetchedCompletionNavigation() {
  Fixture f;
  auto base = std::make_shared<Page>(1, 1000, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, base, ModelOptions{9, 2, 4}));
  auto consumer = take(model->openRange());
  const auto id = consumer.id();
  auto view = take(f.tree->emplace<VirtualTable<int>>(f.root, std::move(consumer), columns()));

  take(setVirtualTableFirst(*f.tree, view, 100));

  auto work = take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));

  check(work && work->range() == RowRange{98, 7}, "request remains exact and within the model budget");

  auto cached = std::make_shared<Page>(1, 1000, RowRange{80, 60});
  const std::weak_ptr<Page> lifetime = cached;

  take(work->complete(cached));
  take(f.scheduler->dispatch());
  work.reset();
  cached.reset();
  take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));

  for (std::uint64_t first = 101; first <= 120; ++first) {
    take(setVirtualTableFirst(*f.tree, view, first));
    check(!take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4})),
          "forward navigation reuses the bounded completion without another request");
  }

  for (std::uint64_t first = 120; first >= 90; --first) {
    take(setVirtualTableFirst(*f.tree, view, first));
    check(!take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4})),
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
  auto binding = take(bindVirtualTable(*router, *f.tree, view));

  take(setVirtualTableSelection(*f.tree, view, RowId{91, 1}));
  take(router->invoke(view, {Action::MOVE_DOWN}));
  take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));
  check(selected(f, view) == RowId{92, 1}, "cached navigation selects the current row's full identity");

  Screen screen;

  take(screen.draw(f, view, {8, 4}));
  check(screen.row(1) == "10  20  ", "cached rows are formatted at their actual current logical indices");

  auto oversized = prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 10});

  check(!oversized && oversized.error() == TreeError::INVALID_ARGUMENT,
        "prefetched storage cannot bypass the synchronous row budget");
  take(setVirtualTableFirst(*f.tree, view, 200));

  auto miss = take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));

  check(miss.has_value(), "cache miss requests the new exact viewport");
  take(screen.draw(f, view, {8, 4}));
  check(screen.row(1) == "... ... ", "cache miss displays pending, not rows from the old viewport");
  take(miss->complete(std::make_shared<Page>(1, 1000, RowRange{190, 60})));
  take(f.scheduler->dispatch());
  take(setVirtualTableFirst(*f.tree, view, 90));
  check(!take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4})) && miss->stopRequested(),
        "reverse after a delayed completion reuses the ready retained page");
  miss.reset();

  take(model->publisher().publish(std::make_shared<Page>(2, 1001, RowRange{}, RangeState::PENDING)));
  take(f.scheduler->dispatch());

  auto revised = take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));

  check(revised && revised->dataRevision() == 2, "new revision requests its own rows, not cached old data");
  check(lifetime.expired(), "old completion storage is released after revision and presentation replacement");
  check(selected(f, view) == RowId{92, 1}, "pending revision does not assign identity from a stale ordinal");
  take(screen.draw(f, view, {8, 4}));
  check(screen.row(1) == "... ... ", "old revision rows are not displayed while the new revision is pending");

  auto wrongCount = revised->complete(std::make_shared<Page>(2, 1000, RowRange{80, 60}));

  check(!wrongCount && wrongCount.error() == SchedulerError::INVALID_ARGUMENT,
        "completion row count must agree with the requesting domain snapshot");
  take(revised->complete(std::make_shared<Page>(2, 1001, RowRange{80, 60})));
  take(f.scheduler->dispatch());
  revised.reset();
  take(prepareVirtualTable(*f.tree, view, take(model->pin()), {8, 4}));
  check(!take(prepareVirtualTable(*f.tree, view, take(model->pin()), {0, 0})),
        "zero viewport does not request or materialize rows");
}

void singleBudgetRetryAndAffinity() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, page, ModelOptions{16, 1, 1}));
  auto list = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns()));
  auto first = take(prepareVirtualTable(*f.tree, list, take(model->pin()), {8, 4}));

  take(setVirtualTableFirst(*f.tree, list, 10));

  auto full = prepareVirtualTable(*f.tree, list, take(model->pin()), {8, 4});

  check(!full && full.error() == TreeError::WOULD_BLOCK, "cancelled caller token retains finite work budget");
  first.reset();

  auto next = take(prepareVirtualTable(*f.tree, list, take(model->pin()), {8, 4}));

  check(next && next->range() == RowRange{8, 7}, "releasing cancelled receipt restores single-slot progress");

  auto pin = take(model->pin());
  std::expected<std::optional<ModelRequest<int>>, TreeError> foreign;
  std::thread worker([&] {
    foreign = prepareVirtualTable(*f.tree, list, pin, {8, 4});
  });

  worker.join();
  check(!foreign && foreign.error() == TreeError::WRONG_THREAD,
        "preparation rejects foreign thread before provider read");
}

void consumersHideRetryAndSelectionIntent() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 100, RowRange{}, RangeState::PENDING);
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  auto a = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns()));
  auto b = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns()));

  take(setVirtualTableFirst(*f.tree, b, 20));

  auto pin = take(model->pin());
  auto ar = take(prepareVirtualTable(*f.tree, a, pin, {8, 4}));
  auto br = take(prepareVirtualTable(*f.tree, b, pin, {8, 4}));

  take(br->complete(std::make_shared<Page>(1, 100, br->range())));
  take(f.scheduler->dispatch());

  auto next = take(model->pin());

  take(prepareVirtualTable(*f.tree, a, next, {8, 4}));
  take(prepareVirtualTable(*f.tree, b, next, {8, 4}));
  take(f.tree->read(a, [](const auto &view) {
    check(view.rangeState() == RangeState::PENDING, "other consumer remains pending");
  }));
  take(f.tree->read(b, [](const auto &view) {
    check(view.rangeState() == RangeState::READY, "completion updates its consumer");
  }));

  auto router = take(EventRouter::create(*f.tree));
  auto binding = take(bindVirtualTable(*router, *f.tree, a));

  take(router->invoke(a, {Action::MOVE_DOWN}));
  take(setVirtualTableSelection(*f.tree, a, RowId{77, 1}));
  take(ar->complete(std::make_shared<Page>(1, 100, ar->range())));
  take(f.scheduler->dispatch());
  take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));
  check(selected(f, a) == RowId{77, 1}, "explicit selection cancels pending navigation intent");
  take(setVirtualTableFirst(*f.tree, a, 40));

  auto hidden_work = take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));

  take(f.tree->setVisible(f.root, false));
  take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));
  check(hidden_work->stopRequested(), "hidden ancestor cancels pending work");
  take(f.tree->read(a, [](const auto &view) {
    check(view.visibleRange().count == 0, "hidden view releases delegates");
  }));
  take(f.tree->setVisible(f.root, true));

  auto retry = take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));

  take(retry->complete(std::make_shared<Page>(1, 100, retry->range(), RangeState::FAILED)));
  take(f.scheduler->dispatch());
  take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));
  take(f.tree->read(a, [](const auto &view) {
    check(view.rangeState() == RangeState::FAILED, "provider error is observable");
  }));
  take(f.tree->remove(b));

  Screen screen;

  take(screen.draw(f, a, {8, 4}));
  check(screen.row(1) == "!   !   ", "provider error placeholder renders explicitly");
  take(retryVirtualTable(*f.tree, a));

  auto retried = take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));

  check(retried.has_value(), "explicit provider retry gives fresh dispatch receipt");
  retried->cancel();

  auto again = take(prepareVirtualTable(*f.tree, a, take(model->pin()), {8, 4}));

  check(again.has_value(), "rejected worker dispatch is retryable after cancellation");
  take(f.tree->remove(a));
  check(again->stopRequested(), "removal cancels worker without joining");
}

struct Record {
  std::string name;
  int amount;
};

struct RecordPage : ModelSnapshot<Record> {
  std::vector<ModelRow<Record>> rows{{{1, 1}, {"old", 1}}, {{2, 1}, {"two", 2}}};
  mutable int reads{};

  explicit RecordPage(std::uint64_t revision) : ModelSnapshot(revision, 2) {
  }

  ModelRows<Record> read(RowRange range) const override {
    ++reads;

    return {RangeState::READY, std::span(rows).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }
};

void coherentTypedColumns() {
  Fixture f;
  auto old = std::make_shared<RecordPage>(1);
  auto next = std::make_shared<RecordPage>(2);

  next->rows[0].value = {"new", 9};

  auto model = take(Model<Record>::create(*f.tree, f.root, old));
  std::vector<TableColumn<Record>> definitions;

  definitions.push_back({{1, 1}, take(GraphemeText::from("Name")), 4, [](const Record &row) {
                           return take(GraphemeText::from(row.name));
                         }});
  definitions.push_back({{2, 1}, take(GraphemeText::from("Qty")), 4, [](const Record &row) {
                           return numberText(row.amount);
                         }});

  auto table = take(f.tree->emplace<VirtualTable<Record>>(f.root, take(model->openRange()), std::move(definitions)));
  auto pin = take(model->pin());
  Screen screen;

  take(prepareVirtualTable(*f.tree, table, pin, {8, 3}));
  take(screen.draw(f, table, {8, 3}, {}, {}, [&] {
    std::thread publisher([&] {
      take(model->publisher().publish(next));
    });

    publisher.join();
  }));
  check(screen.row(1) == "old 1   ", "publication inside real paint frame cannot mix typed columns");
  take(f.scheduler->dispatch());
  take(screen.draw(f, table, {8, 3}));
  check(screen.row(1) == "old 1   ", "prepared presentation owns its pin after publication delivery");
  check(old->reads == 1 && next->reads == 0, "one snapshot read serves every visible typed column");
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 3}));
  take(screen.draw(f, table, {8, 3}));
  check(screen.row(1) == "new 9   " && next->reads == 1, "next preparation switches all columns together");
}

void columnReplacementAllocation() {
  Fixture f;
  auto page = std::make_shared<Page>(1, 10, RowRange{0, 10});
  auto model = take(Model<int>::create(*f.tree, f.root, page));
  auto table = take(f.tree->emplace<VirtualTable<int>>(f.root, take(model->openRange()), columns()));
  auto replacement = columns();
  Screen screen;

  replacement[0].header = take(GraphemeText::from("New"));
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}));

  bool completed{};
  int failures_seen{};

  for (std::ptrdiff_t budget = 0; budget < 256 && !completed; ++budget) {
    allocation_test::remaining = budget;

    const auto result = setVirtualTableColumns(*f.tree, table, replacement);

    allocation_test::remaining = -1;

    if (result) {
      completed = true;
    } else {
      ++failures_seen;
      check(result.error() == TreeError::NO_MEMORY, "column copy allocation failure is explicit");
      take(screen.draw(f, table, {8, 4}));
      check(screen.row(0) == "A   B   ", "failed column copy preserves prepared header");
    }
  }

  check(completed && failures_seen > 0, "column replacement recovers from allocation failure");
  take(prepareVirtualTable(*f.tree, table, take(model->pin()), {8, 4}));
  take(screen.draw(f, table, {8, 4}));
  check(screen.row(0) == "New B   ", "successful column copy commits new header");
}
} // namespace

int main() {
  cachedFailuresAreNotHidden();
  prefetchedCompletionNavigation();
  boundedColumns();
  columnReplacementAllocation();
  coherentTypedColumns();
  singleBudgetRetryAndAffinity();
  consumersHideRetryAndSelectionIntent();
  geometryAndSelection();
  narrowEmptyAndBudgets();
  allocationTransactionAndIdle();
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
