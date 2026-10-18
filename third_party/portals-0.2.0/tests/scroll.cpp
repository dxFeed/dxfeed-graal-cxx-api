// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <cmath>
#include <cstdlib>
#include <format>
#include <iostream>
#include <map>
#include <portals/virtual_table.hpp>
#include <source_location>
#include <thread>

namespace {
using namespace portals;
int failures{};

void check(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E>
T take(std::expected<T, E> result, std::source_location location = std::source_location::current()) {
  if (!result) {
    std::cerr << "fixture failed at " << location.line() << '\n';
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

struct Page : ModelSnapshot<int> {
  enum class Fault {
    NONE,
    UNSUPPORTED,
    BOUNDS,
    THROW,
    ALLOCATION
  };
  std::vector<ModelRow<int>> rows;
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t> index;
  Fault fault{};
  mutable std::size_t reads{};
  mutable std::size_t lookups{};

  Page(std::uint64_t revision, std::initializer_list<std::uint64_t> ids) : ModelSnapshot(revision, ids.size()) {
    for (auto id : ids) {
      index.emplace(std::pair{id, 1ULL}, rows.size());
      rows.push_back({{id, 1}, static_cast<int>(id)});
    }
  }

  ModelRows<int> read(RowRange range) const override {
    ++reads;

    return {RangeState::READY, std::span(rows).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }

  RowLookup locateRow(RowId id) const override {
    ++lookups;

    switch (fault) {
    case Fault::UNSUPPORTED:
      return {};
    case Fault::BOUNDS:
      return {RowLookupState::FOUND, rowCount()};
    case Fault::THROW:
      throw std::runtime_error("lookup failed");
    case Fault::ALLOCATION:
      throw std::bad_alloc();
    case Fault::NONE:
      break;
    }

    auto found = index.find({id.value, id.generation});

    return found == index.end() ? RowLookup{RowLookupState::NOT_FOUND, 0}
                                : RowLookup{RowLookupState::FOUND, found->second};
  }
};

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
  std::shared_ptr<Page> page = std::make_shared<Page>(1, std::initializer_list<std::uint64_t>{1, 2, 3, 4, 5, 6});
  std::unique_ptr<Model<int>> model = take(Model<int>::create(*tree, root, page));
  std::size_t formats{};
  Node<VirtualList<int>> list =
    take(tree->emplace<VirtualList<int>>(root, take(model->openRange()), [this](const auto &row) {
      ++formats;

      return take(GraphemeText::from(std::format("{}", row.value)));
    }));
  Node<VirtualTable<int>> table = take(tree->emplace<VirtualTable<int>>(
    root, take(model->openRange()),
    std::vector<TableColumn<int>>{{{1, 1}, take(GraphemeText::from("ID")), 4, [this](const int &value) {
                                     ++formats;

                                     return take(GraphemeText::from(std::format("{}", value)));
                                   }}}));

  void prepare() {
    const auto pin = take(model->pin());

    take(prepareVirtualList(*tree, list, pin, {8, 2}));
    take(prepareVirtualTable(*tree, table, pin, {8, 3}));
  }

  void publish(std::shared_ptr<Page> next) {
    page = std::move(next);
    take(model->publisher().publish(page));
    take(scheduler->dispatch());
  }

  void expectPosition(std::uint64_t first, RowId anchor) {
    auto verify = [&](const auto &view) {
      check(view.firstRow() == first, "both views preserve the expected raster index");
      check(view.anchorRow() == anchor, "both views preserve the full stable anchor identity");
    };

    take(tree->read(list, verify));
    take(tree->read(table, verify));
  }
};

void stableAnchor() {
  Fixture f;

  take(setVirtualListFirst(*f.tree, f.list, 2));
  take(setVirtualTableFirst(*f.tree, f.table, 2));
  take(setVirtualListSelection(*f.tree, f.list, RowId{5, 1}));
  take(setVirtualTableSelection(*f.tree, f.table, RowId{5, 1}));
  f.prepare();
  f.publish(std::make_shared<Page>(2, std::initializer_list<std::uint64_t>{9, 8, 1, 2, 3, 4, 5, 6}));
  f.prepare();
  f.expectPosition(4, {3, 1});
  f.publish(std::make_shared<Page>(3, std::initializer_list<std::uint64_t>{8, 2, 3, 4, 5, 6}));
  f.prepare();
  f.expectPosition(2, {3, 1});
  f.publish(std::make_shared<Page>(4, std::initializer_list<std::uint64_t>{8, 2, 4, 5, 6}));
  f.prepare();
  f.expectPosition(2, {4, 1});
  take(f.tree->read(f.list, [](const auto &view) {
    check(view.selection() == RowId{5, 1}, "anchor deletion does not rewrite selection");
  }));
  take(f.tree->read(f.table, [](const auto &view) {
    check(view.selection() == RowId{5, 1}, "table anchor is distinct from selection");
  }));
  f.publish(std::make_shared<Page>(5, std::initializer_list<std::uint64_t>{8}));
  f.prepare();
  f.expectPosition(0, {8, 1});
}

void quantizationAndBounds() {
  ScrollState state;

  for (int step = 0; step < 3; ++step) {
    check(!take(state.moveBy(0.25, 100)), "fraction does not move raster early");
  }

  check(take(state.moveBy(0.25, 100)) && state.rasterOffset() == 1 && state.fraction() == 0,
        "four quarter wheel steps make one raster row");
  check(take(state.moveBy(-0.25, 100)) && state.rasterOffset() == 0 && state.fraction() == 0.75,
        "negative movement uses floor quantization");
  take(state.moveBy(-100, 100));
  check(state.rasterOffset() == 0 && state.fraction() == 0, "start discards overscroll");
  take(state.moveBy(std::numeric_limits<double>::max(), 100));
  check(state.rasterOffset() == 99 && state.fraction() == 0, "huge finite positive input saturates safely");
  take(state.moveBy(-std::numeric_limits<double>::max(), 100));
  check(state.rasterOffset() == 0, "huge negative input saturates safely");

  take(state.setOffset(1));
  take(state.moveBy(-std::numeric_limits<double>::min(), 100));
  check(state.rasterOffset() == 0 && state.fraction() < 1, "sub-ULP negative input keeps canonical fraction");

  const auto max = std::numeric_limits<std::uint64_t>::max();

  take(state.setOffset(max - 3, 0.5));
  take(state.moveBy(0.5, max));
  check(state.rasterOffset() == max - 2, "exact integer storage survives above 2^53");
  take(state.moveBy(1, max));
  check(state.rasterOffset() == max - 1 && state.fraction() == 0, "uint64 upper boundary does not wrap");
  take(state.moveBy(-0.5, max));
  check(state.rasterOffset() == max - 2 && state.fraction() == 0.5, "large-index reverse fraction is preserved");

  const auto invalid = state.moveBy(std::numeric_limits<double>::infinity(), max);
  const auto bad = state.setOffset(12, std::numeric_limits<double>::quiet_NaN());

  check(!invalid && !bad && state.rasterOffset() == max - 2 && state.fraction() == 0.5,
        "nonfinite input leaves both parts unchanged");
  check(!state.setOffset(0, -0.1) && !state.setOffset(0, 1), "fraction range is explicit");
  state.clamp(0);
  check(state.rasterOffset() == 0 && state.fraction() == 0, "empty models reset scroll");

  // Dyadic inputs give an exact independent reference throughout this deterministic trace.
  double reference{};

  for (int step = 0; step < 4096; ++step) {
    const double delta = static_cast<double>((step * 37) % 19 - 9) / 4;

    reference = std::clamp(reference + delta, 0.0, 99.0);
    take(state.moveBy(delta, 100));
    check(state.rasterOffset() == static_cast<std::uint64_t>(std::floor(reference)) &&
            state.fraction() == reference - std::floor(reference),
          "wheel trace matches independent floor reference");
  }

  take(state.setOffset(20, 0.5));
  check(!take(state.ensureVisible(22, 4, 100)) && state.fraction() == 0.5, "visible target keeps fraction");
  check(take(state.ensureVisible(30, 4, 100)) && state.rasterOffset() == 27, "reveal end minimally");
  check(take(state.ensureVisible(2, 4, 100)) && state.rasterOffset() == 2, "reveal start minimally");
  check(!state.ensureVisible(100, 4, 100), "out-of-range ensure-visible is explicit");
  check(!take(state.ensureVisible(50, 0, 100)) && state.rasterOffset() == 2, "zero viewport cannot reveal");
}

void fractionalViewsAndEnsureVisible() {
  Fixture f;

  check(!scrollVirtualList(*f.tree, f.list, 0.25), "unprepared scroll reports would-block");
  f.prepare();
  take(f.tree->consumeFrameRequest());

  const auto formats = f.formats;
  const auto reads = f.page->reads;

  for (int step = 0; step < 3; ++step) {
    take(scrollVirtualList(*f.tree, f.list, 0.25));
    take(scrollVirtualTable(*f.tree, f.table, 0.25));
    f.prepare();
  }

  check(f.formats == formats && f.page->reads == reads, "fraction-only changes do no provider or formatting work");
  check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE, "fraction-only input schedules no frame");
  take(scrollVirtualList(*f.tree, f.list, 0.25));
  take(scrollVirtualTable(*f.tree, f.table, 0.25));
  f.prepare();
  f.expectPosition(1, {2, 1});

  const auto pin = take(f.model->pin());

  check(take(ensureVirtualListVisible(*f.tree, f.list, pin, {6, 1})).state == RowLookupState::FOUND,
        "list ensure-visible resolves identity");
  check(take(ensureVirtualTableVisible(*f.tree, f.table, pin, {6, 1})).state == RowLookupState::FOUND,
        "table ensure-visible resolves identity excluding its fixed header");
  f.prepare();
  f.expectPosition(4, {5, 1});
  check(take(ensureVirtualListVisible(*f.tree, f.list, pin, {6, 2})).state == RowLookupState::NOT_FOUND,
        "generation mismatch is not another entity");
  check(take(ensureVirtualTableVisible(*f.tree, f.table, pin, {99, 1})).state == RowLookupState::NOT_FOUND,
        "absent identity is explicit");

  VirtualListOptions listOptions;
  VirtualTableOptions tableOptions;

  listOptions.row_height = 3;
  tableOptions.row_height = 3;
  take(setVirtualListOptions(*f.tree, f.list, listOptions));
  take(setVirtualTableOptions(*f.tree, f.table, tableOptions));
  f.prepare();
  take(ensureVirtualListVisible(*f.tree, f.list, pin, {1, 1}));
  take(ensureVirtualTableVisible(*f.tree, f.table, pin, {1, 1}));
  f.prepare();
  take(f.tree->read(f.list, [](const auto &view) {
    check(view.firstRow() == 0, "ensure-visible reveals the start of a row taller than the list viewport");
  }));
  take(f.tree->read(f.table, [](const auto &view) {
    check(view.firstRow() == 0, "ensure-visible reveals the start of a row taller than the table body");
  }));
  std::expected<void, TreeError> foreign;
  std::thread worker([&] {
    foreign = scrollVirtualList(*f.tree, f.list, 1);
  });

  worker.join();
  check(!foreign && foreign.error() == TreeError::WRONG_THREAD, "scroll uses existing owner-thread check");
}

struct Sink : PaintContext {
  std::vector<RenderCommand> commands;

  std::expected<void, TreeError> submit(std::span<const RenderCommand> input) override {
    for (const auto &command : input) {
      commands.emplace_back(command);
    }

    return {};
  }
};

void hiddenSubtreeAndRaster() {
  Fixture f;

  take(setVirtualListFirst(*f.tree, f.list, 2));
  take(setVirtualTableFirst(*f.tree, f.table, 2));
  f.prepare();
  take(f.tree->setVisible(f.root, false));
  f.prepare();

  const auto formats = f.formats;

  f.publish(std::make_shared<Page>(2, std::initializer_list<std::uint64_t>{9, 1, 2, 3, 4, 5, 6}));
  f.prepare();
  check(take(f.model->pin()).dataRevision() == 2 && f.formats == formats && f.page->reads == 0,
        "hidden subtree continues model publication without provider reads or formatting");

  Sink sink;

  take(f.tree->frame([&] {
    take(f.tree->paint(f.list, sink));
    take(f.tree->paint(f.table, sink));
  }));
  check(sink.commands.empty(), "hidden subtree submits no paint commands");
  take(f.tree->setVisible(f.root, true));
  f.prepare();
  f.expectPosition(3, {3, 1});
  take(scrollVirtualList(*f.tree, f.list, 0.5));

  auto bounds = take(CellRect::from({}, {8, 2}));

  take(f.tree->frame([&] {
    take(f.tree->arrange(f.root, bounds, bounds));
    take(layoutTree(*f.tree, f.list, bounds, bounds));
    take(f.tree->paint(f.list, sink));

    const auto buffer = take(renderFullFrame({8, 2}, sink.commands));

    check(!sink.commands.empty(), "visible list produces raster commands after fractional input");
    check(take(buffer.at({0, 0})).glyph == "3" && take(buffer.at({0, 1})).glyph == "4",
          "fractional scroll keeps text on integral cell rows");
  }));
}

struct PaintCounter : Widget {
  int *calls;

  explicit PaintCounter(int &count) : calls(&count) {
  }

  std::expected<void, TreeError> paint(PaintContext &) override {
    ++*calls;

    return {};
  }
};

void viewportCulling() {
  Fixture f;
  int calls{};
  auto counter = take(f.tree->emplace<PaintCounter>(f.root, calls));
  auto viewport = take(CellRect::from({}, {8, 3}));
  auto offscreen = take(CellRect::from({20, 0}, {8, 3}));
  Sink sink;

  take(f.tree->frame([&] {
    const auto invalid = f.tree->paint(counter, sink);

    check(!invalid && invalid.error() == TreeError::INVALID_GEOMETRY, "culling cannot hide invalid geometry");
    take(f.tree->arrange(f.root, viewport, viewport));
    take(f.tree->arrange(counter, offscreen, viewport));
    take(f.tree->paint(counter, sink));
  }));
  check(calls == 0, "offscreen widget paint hook is culled");
  f.publish(std::make_shared<Page>(2, std::initializer_list<std::uint64_t>{7, 8}));
  check(take(f.model->pin()).dataRevision() == 2, "offscreen culling never stops model publications");
  take(f.tree->frame([&] {
    take(f.tree->arrange(f.root, viewport, viewport));
    take(f.tree->arrange(counter, viewport, viewport));
    take(f.tree->paint(counter, sink));
  }));
  check(calls == 1, "returning into viewport restores paint");
}

void lookupFailureAndLegacy() {
  Fixture f;

  take(setVirtualListFirst(*f.tree, f.list, 2));
  take(setVirtualTableFirst(*f.tree, f.table, 2));
  f.prepare();

  for (const auto fault : {Page::Fault::BOUNDS, Page::Fault::THROW, Page::Fault::ALLOCATION}) {
    auto page =
      std::make_shared<Page>(f.page->revision() + 1, std::initializer_list<std::uint64_t>{9, 1, 2, 3, 4, 5, 6});

    page->fault = fault;
    f.publish(page);

    const auto pin = take(f.model->pin());
    const auto list = prepareVirtualList(*f.tree, f.list, pin, {8, 2});
    const auto table = prepareVirtualTable(*f.tree, f.table, pin, {8, 3});
    const auto error = fault == Page::Fault::BOUNDS  ? TreeError::INVALID_ARGUMENT
                       : fault == Page::Fault::THROW ? TreeError::CALLBACK_FAILED
                                                     : TreeError::NO_MEMORY;

    check(!list && !table && list.error() == error && table.error() == error,
          "lookup failures reject entire preparation with explicit error");
    f.expectPosition(2, {3, 1});
  }

  auto legacy =
    std::make_shared<Page>(f.page->revision() + 1, std::initializer_list<std::uint64_t>{9, 1, 2, 3, 4, 5, 6});

  legacy->fault = Page::Fault::UNSUPPORTED;
  f.publish(legacy);
  f.prepare();
  f.expectPosition(2, {2, 1});

  const auto pin = take(f.model->pin());

  check(take(ensureVirtualListVisible(*f.tree, f.list, pin, {6, 1})).state == RowLookupState::UNSUPPORTED &&
          take(ensureVirtualTableVisible(*f.tree, f.table, pin, {6, 1})).state == RowLookupState::UNSUPPORTED,
        "legacy providers report unsupported identity lookup instead of claiming missing row");
  check(!pin.locateRow({0, 1}), "invalid lookup identity is rejected before provider call");
  auto foreignPage = std::make_shared<Page>(99, std::initializer_list<std::uint64_t>{9, 1, 2, 3, 4, 5, 6});
  auto foreignModel = take(Model<int>::create(*f.tree, f.root, foreignPage));
  const auto foreignPin = take(foreignModel->pin());
  const auto foreignList = prepareVirtualList(*f.tree, f.list, foreignPin, {8, 2});
  const auto foreignTable = prepareVirtualTable(*f.tree, f.table, foreignPin, {8, 3});

  check(!foreignList && !foreignTable && foreignList.error() == TreeError::STALE_NODE &&
          foreignTable.error() == TreeError::STALE_NODE,
        "foreign pins are rejected");
  check(foreignPage->lookups == 0, "foreign provider is never called before consumer validation");
}
} // namespace

int main() {
  quantizationAndBounds();
  stableAnchor();
  fractionalViewsAndEnsureVisible();
  hiddenSubtreeAndRaster();
  lookupFailureAndLegacy();
  viewportCulling();

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
