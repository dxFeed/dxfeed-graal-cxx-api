// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/test_terminal.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <portals/action.hpp>
#include <portals/grid.hpp>
#include <type_traits>

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

using namespace portals;

namespace {
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

CellRect rect(int x, int y, int width, int height) {
  return take(CellRect::from({x, y}, {width, height}));
}

AxisSize fixed(int value) {
  return {value, value, value, SizeMode::FIXED};
}

struct Leaf : SizedWidget {
  CellSize desired;

  explicit Leaf(CellSize size, SizePolicy policy = {}) : SizedWidget(policy), desired(size) {
  }

  std::expected<CellSize, TreeError> measure(CellSize) override {
    return desired;
  }
};

void nestedIdentityAndSpans() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  GridOptions outer_options;

  outer_options.columns = {fixed(3), fixed(5)};
  outer_options.rows = {fixed(2), fixed(3)};
  outer_options.column_gap = 1;

  auto outer = take(tree->emplace<Grid>(root, outer_options));
  auto first = take(tree->emplace<Leaf>(outer, CellSize{1, 1}));
  auto inner = take(tree->emplace<Grid>(outer));
  auto spanning = take(tree->emplace<Leaf>(outer, CellSize{1, 1}));
  auto nested = take(tree->emplace<Leaf>(inner, CellSize{1, 1}));

  outer_options.cells = {{first.id(), 0, 0}, {inner.id(), 0, 1}, {spanning.id(), 1, 0, 1, 2}};

  GridOptions inner_options;

  inner_options.columns = {fixed(5)};
  inner_options.rows = {fixed(2)};
  inner_options.cells = {{nested.id(), 0, 0, 1, 1, LayoutAlign::CENTER, LayoutAlign::END}};
  take(setGridOptions(*tree, outer, outer_options));
  take(setGridOptions(*tree, inner, inner_options));

  auto draw = [&] {
    take(tree->frame([&] {
      take(layoutTree(*tree, root, rect(0, 0, 9, 5), rect(0, 0, 9, 5)));
    }));
  };

  draw();
  check(take(tree->inspect(inner)).geometry.local == rect(4, 0, 5, 2), "nested grid uses explicit cell");
  check(take(tree->inspect(spanning)).geometry.local == rect(0, 2, 9, 3), "span includes internal gap");
  check(take(tree->inspect(nested)).geometry.surface == rect(6, 1, 1, 1), "nested alignment translates once");
  take(tree->setVisible(first, false));

  draw();
  check(take(tree->inspect(inner)).geometry.local == rect(4, 0, 5, 2), "hiding sibling preserves identity placement");
  outer_options.cells.back().column_span = 3;

  auto rejected = setGridOptions(*tree, outer, outer_options);

  check(!rejected && rejected.error() == TreeError::INVALID_ARGUMENT, "out-of-range span rejected");
  outer_options.cells.back().column_span = 0;
  rejected = setGridOptions(*tree, outer, outer_options);

  check(!rejected && rejected.error() == TreeError::INVALID_ARGUMENT, "zero span rejected");
}

void tracksBoundsAndErrors() {
  const NodeId a{1, 1, 1}, b{1, 2, 1}, c{1, 3, 1};
  GridOptions options;

  options.columns = {AxisSize{}, AxisSize{}};
  options.rows = {fixed(2)};
  options.column_gap = 1;
  options.cells = {{a, 0, 0}, {b, 0, 1}, {c, 0, 0, 1, 2}};

  const std::array children{MeasuredChild{{2, 1}, {}, a}, MeasuredChild{{1, 1}, {}, b}, MeasuredChild{{9, 1}, {}, c}};

  Grid grid(options);

  check(take(grid.measureChildren(children)) == CellSize{9, 2}, "span grows intrinsic tracks beyond singles");

  auto placed = take(grid.arrangeChildren(children, {9, 2}));

  check(placed[0] == rect(0, 0, 5, 2) && placed[1] == rect(6, 0, 3, 2), "span deficit remainder follows track order");

  options.columns[0].maximum = 2;
  placed = take(Grid(options).arrangeChildren(children, {9, 2}));
  check(placed[0].size().width == 2 && placed[1] == rect(3, 0, 6, 2), "span demand redistributes around track cap");

  options.columns = {{2, 7, 9, SizeMode::AUTOMATIC}, {3, 7, 9, SizeMode::FLEX}};
  placed = take(Grid(options).arrangeChildren(children, {4, 2}));
  check(placed[0].size().width == 2 && placed[1] == rect(3, 0, 3, 2), "shortage preserves track minima and gaps");

  for (int width = 0; width <= 17; ++width) {
    options.column_gap = 0;
    options.columns = {{0, 0, 4, SizeMode::FLEX}, {0, 0, 9, SizeMode::FLEX}};
    placed = take(Grid(options).arrangeChildren(children, {width, 2}));
    check(placed[0].size().width + placed[1].size().width == std::min(width, 13),
          "flex fills width until track maxima");
    check(placed[1].origin().x == placed[0].size().width, "all property cases have contiguous tracks");
  }

  options.columns = {fixed(2), fixed(3)};
  options.column_gap = std::numeric_limits<std::int32_t>::max();

  auto failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::INVALID_GEOMETRY, "track/gap arithmetic checked");

  auto bad_rects = Grid(options).arrangeChildren(children, {4, 2});

  check(!bad_rects && bad_rects.error() == TreeError::INVALID_GEOMETRY, "arrange endpoint overflow checked");

  options.column_gap = 0;
  options.max_tracks_per_axis = 1;
  failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::NODE_LIMIT, "configured track budget enforced");
  options.max_tracks_per_axis = 2;
  options.max_placements = 2;
  failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::NODE_LIMIT, "placement budget includes all entries");
  options.max_placements = 3;
  options.cells[2].node = a;
  failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::INVALID_ARGUMENT, "duplicate identity rejected");
  options.cells[2].node = c;
  options.cells[2].row = std::numeric_limits<std::size_t>::max();
  failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::INVALID_ARGUMENT, "index overflow rejected before addition");
  options.cells[2].row = 0;
  options.cells[2].row_span = std::numeric_limits<std::size_t>::max();
  failure = Grid(options).measureChildren(children);

  check(!failure && failure.error() == TreeError::INVALID_ARGUMENT, "span overflow rejected before addition");
}

void alignmentAndOverlay() {
  const NodeId id{1, 1, 1};

  const std::array children{MeasuredChild{{2, 1}, {}, id}};
  GridOptions options;

  options.columns = {fixed(7)};
  options.rows = {fixed(4)};
  options.cells = {{id, 0, 0, 1, 1, LayoutAlign::CENTER, LayoutAlign::CENTER}};
  check(take(Grid(options).arrangeChildren(children, {7, 4}))[0] == rect(2, 1, 2, 1),
        "odd center remainder after child on both axes");

  OverlayOptions overlay_options{{1, 1, 1, 1}, LayoutAlign::END, LayoutAlign::CENTER};
  Overlay overlay(overlay_options);

  check(take(overlay.measureChildren(children)) == CellSize{4, 3}, "overlay desired size includes padding");
  check(take(overlay.arrangeChildren(children, {9, 6}))[0] == rect(6, 2, 2, 1), "overlay aligns inside padding");

  auto constrained = children;

  constrained[0].policy.width = {4, 5, 6, SizeMode::FIXED};
  constrained[0].policy.height = {2, 3, 3, SizeMode::AUTOMATIC};
  check(take(Overlay{}.arrangeChildren(constrained, {2, 1}))[0] == rect(0, 0, 5, 2),
        "child fixed/minimum survive slot shortage");
  check(take(Overlay{}.arrangeChildren(constrained, {12, 9}))[0] == rect(0, 0, 5, 3),
        "stretch observes fixed and maximum");

  overlay_options.padding.left = std::numeric_limits<std::int32_t>::max();

  auto failure = Overlay(overlay_options).measureChildren(children);

  check(!failure && failure.error() == TreeError::INVALID_GEOMETRY, "padding overflow checked");
  overlay_options.padding.left = -1;
  failure = Overlay(overlay_options).childAvailable({3, 3});

  check(!failure && failure.error() == TreeError::INVALID_ARGUMENT, "negative overlay padding rejected");
  constrained[0].policy.height.minimum = -1;

  auto invalid = Overlay{}.arrangeChildren(constrained, {3, 3});

  check(!invalid && invalid.error() == TreeError::INVALID_ARGUMENT, "invalid child bounds rejected");
}

struct PaintLeaf : Leaf {
  GraphemeText text;
  CellSize allocated{};
  int presses{};

  PaintLeaf(std::string_view value, SizePolicy policy = {})
      : Leaf({static_cast<int>(value.size()), 1}, policy), text(take(GraphemeText::from(value))) {
  }

  std::expected<void, TreeError> arrange(CellRect local) override {
    allocated = local.size();

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    const auto clip = rect(0, 0, allocated.width, allocated.height);

    for (int y = 0; y < allocated.height; ++y) {
      const RenderCommand command = TextCommand{{0, y}, text, clip};
      auto result = context.submit(std::span(&command, 1));

      if (!result) {
        return result;
      }
    }

    return {};
  }
};

struct Sink : PaintContext {
  NodeGeometry geometry;
  std::vector<RenderCommand> &commands;

  Sink(NodeGeometry value, std::vector<RenderCommand> &output) : geometry(value), commands(output) {
  }

  std::expected<void, TreeError> submit(std::span<const RenderCommand> input) override {
    for (const auto &command : input) {
      const auto &text = std::get<TextCommand>(command);
      auto origin = take(translated(text.origin, geometry.surface.origin()));
      auto clip = take(text.clip.translated(geometry.surface.origin())).intersection(geometry.clip);

      commands.emplace_back(TextCommand{origin, text.text, clip});
    }

    return {};
  }
};

void paintTree(WidgetTree &tree, Node<Widget> node, std::vector<RenderCommand> &commands) {
  auto snapshot = take(tree.inspect(node));

  if (!snapshot.visible || !snapshot.geometry.valid) {
    return;
  }

  Sink sink(snapshot.geometry, commands);

  take(tree.paint(node, sink));

  for (auto child : snapshot.children) {
    paintTree(tree, child, commands);
  }
}

void renderedClippingAndHitOrder() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto router = take(EventRouter::create(*tree));
  GridOptions options;

  options.columns = {fixed(3), fixed(4)};
  options.rows = {fixed(2)};
  options.padding = {1, 1, 0, 0};

  auto grid = take(tree->emplace<Grid>(root, options));
  auto base = take(tree->emplace<PaintLeaf>(grid, "AAAAAAA"));
  auto overlay = take(tree->emplace<Overlay>(grid));
  auto lower = take(tree->emplace<PaintLeaf>(overlay, "BBBB"));
  SizePolicy front_policy;

  front_policy.width = fixed(2);

  auto front = take(tree->emplace<PaintLeaf>(overlay, "CC", front_policy));

  options.cells = {{overlay.id(), 0, 1}, {base.id(), 0, 0, 1, 2}};
  take(setGridOptions(*tree, grid, options));

  auto binding = take(router->bind(front, [](PaintLeaf &leaf, const RoutedEvent &event) {
    if (std::holds_alternative<MouseInputEvent>(event)) {
      ++leaf.presses;

      return EventReply::HANDLED;
    }

    return EventReply::IGNORED;
  }));
  test_support::FakeClock clock;
  test_support::TestTerminal terminal(clock, {6, 3});
  FrameWriter writer(terminal);

  auto draw = [&](CellSize size) {
    std::optional<CellBuffer> buffer;
    const auto viewport = take(CellRect::from({}, size));

    terminal.resize(size);
    take(tree->invalidate(root, Invalidation::LAYOUT));
    take(tree->frame([&] {
      take(layoutTree(*tree, root, viewport, viewport));

      std::vector<RenderCommand> commands;

      paintTree(*tree, root, commands);
      buffer.emplace(take(renderFullFrame(size, commands)));
    }));
    take(writer.submit(std::move(*buffer)));

    auto output = writer.flush(TerminalDeadline::max());

    check(output.status == TerminalStatus::PROGRESS && !output.error && !writer.pending(),
          "actual writer completes clipped frame");
  };
  auto expect = [&](std::span<const std::string_view> rows) {
    auto snapshot = terminal.snapshot();

    for (int y = 0; y < snapshot.size.height; ++y) {
      std::string actual;

      for (int x = 0; x < snapshot.size.width; ++x) {
        const auto &glyph = snapshot.cells[static_cast<std::size_t>(y * snapshot.size.width + x)].glyph;
        Node<Widget> expected;

        actual += glyph;

        if (glyph == "A") {
          expected = base;
        } else if (glyph == "B") {
          expected = lower;
        } else if (glyph == "C") {
          expected = front;
        } else {
          continue;
        }

        check(take(router->hitTest({x, y})) == expected, "terminal glyph hits same arranged node");
      }

      check(actual == rows[static_cast<std::size_t>(y)], "independent terminal snapshot equals literal rows");
    }
  };

  draw({6, 3});

  const std::array<std::string_view, 3> narrow{"      ", " AAACC", " AAACC"};

  expect(narrow);
  check(take(tree->inspect(lower)).geometry.surface == rect(4, 1, 4, 2), "minimum tracks overflow narrow grid");
  check(take(tree->inspect(lower)).geometry.clip == rect(4, 1, 2, 2), "ancestor clip shared by paint and input");
  check(take(router->hitTest({6, 1})) == Node<Widget>{}, "half-open viewport edge is never an input target");

  auto events = terminal.input("\x1b[<0;5;2M");

  check(events.size() == 1 && take(router->dispatch(events[0])).handler == front.id(),
        "real decoded mouse chooses overlay front");
  take(tree->read(front, [](const PaintLeaf &leaf) {
    check(leaf.presses == 1, "typed handler invoked once");
  }));

  take(tree->frame([&] {
    auto rejected = setOverlayOptions(*tree, overlay, {});

    check(!rejected && rejected.error() == TreeError::WOULD_BLOCK, "overlay settings cannot change during a frame");
  }));
  take(setOverlayOptions(*tree, overlay, {}));
  check(!take(tree->inspect(front)).geometry.valid, "overlay setter invalidates arranged descendants");

  draw({8, 3});

  const std::array<std::string_view, 3> wide{"        ", " AAACCBB", " AAACCBB"};

  expect(wide);
  take(tree->setVisible(front, false));

  draw({8, 3});

  const std::array<std::string_view, 3> hidden{"        ", " AAABBBB", " AAABBBB"};

  expect(hidden);
}

struct GuardedGrid : Grid {
  WidgetTree &tree;
  Node<Widget> root;
  mutable bool guarded{};

  GuardedGrid(WidgetTree &owner, Node<Widget> node) : tree(owner), root(node) {
  }

  std::expected<CellSize, TreeError> measureChildren(std::span<const MeasuredChild> children) const override {
    const auto mutation = tree.setEnabled(root, false);
    const auto reentry = layoutTree(tree, root, {}, {});

    guarded =
      !mutation && mutation.error() == TreeError::WOULD_BLOCK && !reentry && reentry.error() == TreeError::WOULD_BLOCK;

    return Grid::measureChildren(children);
  }
};

void stalePlacementsAndPureMeasure() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto grid = take(tree->emplace<GuardedGrid>(root, *tree, root));
  auto original = take(tree->emplace<Leaf>(grid, CellSize{2, 1}));
  GridOptions options;

  options.cells = {{original.id()}};
  take(setGridOptions(*tree, grid, options));

  auto run = [&] {
    std::expected<void, TreeError> result;

    take(tree->frame([&] {
      result = layoutTree(*tree, root, rect(0, 0, 5, 3), rect(0, 0, 5, 3));
    }));

    return result;
  };

  take(run());
  take(tree->read(grid, [](const GuardedGrid &value) {
    check(value.guarded, "layout callback mutation/reentry rejected");
  }));
  check(take(tree->inspect(root)).enabled, "measure left tree presentation unchanged");
  take(tree->remove(original));

  auto replacement = take(tree->emplace<Leaf>(grid, CellSize{2, 1}));
  auto failed = run();

  check(!failed && failed.error() == TreeError::INVALID_ARGUMENT, "stale placement never assigns replacement identity");
  check(!take(tree->inspect(replacement)).geometry.valid, "failed pass exposes no replacement geometry");
  options.cells.push_back({replacement.id()});
  take(setGridOptions(*tree, grid, options));

  take(run());
  options.cells[0].column_span = 2;

  auto invalid = setGridOptions(*tree, grid, options);

  check(!invalid && invalid.error() == TreeError::INVALID_ARGUMENT, "stale entry structural span still validated");
  check(take(tree->inspect(replacement)).geometry.valid, "invalid setter preserves preceding configuration/geometry");
}

void verticalSpanAndWeightedTracks() {
  GridOptions options;

  options.columns = {{0, 0, 20, SizeMode::FLEX, 1}, {0, 0, 20, SizeMode::FLEX, 2}};
  options.rows = {fixed(2), fixed(3)};
  options.row_gap = 1;
  options.cells = {{{1, 1, 1}, 0, 0, 2, 1}, {{1, 2, 1}, 0, 1, 2, 1}};

  const std::array children{MeasuredChild{{1, 1}, {}, {1, 1, 1}}, MeasuredChild{{1, 1}, {}, {1, 2, 1}}};

  auto placed = take(Grid(options).arrangeChildren(children, {10, 6}));

  check(placed[0] == rect(0, 0, 4, 6) && placed[1] == rect(4, 0, 6, 6),
        "weighted columns and vertical spans share deterministic geometry");
  options.rows.assign(128, fixed(1));
  options.columns.assign(128, fixed(1));
  options.row_gap = 0;
  options.cells[0].row_span = 128;
  options.cells[0].column_span = 128;
  check(take(Grid(options).measureChildren(children)) == CellSize{128, 128},
        "sparse full-grid span uses only axis tracks");
}

void cappedSpanRemainders() {
  constexpr std::array EXPECTED_SIZES{std::array{1, 2, 2}, std::array{2, 1, 2}, std::array{2, 2, 1}};

  for (bool vertical : {false, true}) {
    for (std::size_t capped = 0; capped < EXPECTED_SIZES.size(); ++capped) {
      GridOptions options;
      std::vector<AxisSize> axis(3);
      std::array<MeasuredChild, 4> children{};

      axis[capped].maximum = 1;
      options.columns = vertical ? std::vector{fixed(1)} : axis;
      options.rows = vertical ? axis : std::vector{fixed(1)};

      for (std::size_t i = 0; i < 3; ++i) {
        children[i].node = {1, i + 1, 1};
        options.cells.push_back({children[i].node, vertical ? i : 0, vertical ? 0 : i});
      }

      children[3].node = {1, 4, 1};
      children[3].desired = vertical ? CellSize{1, 5} : CellSize{5, 1};
      options.cells.push_back({children[3].node, 0, 0, vertical ? 3u : 1u, vertical ? 1u : 3u});

      const Grid grid(options);
      const auto placed = take(grid.arrangeChildren(children, children[3].desired));
      int origin{};

      check(take(grid.measureChildren(children)) == children[3].desired, "capped span retains total demand");

      for (std::size_t i = 0; i < 3; ++i) {
        const auto extent = EXPECTED_SIZES[capped][i];
        const auto expected = vertical ? rect(0, origin, 1, extent) : rect(origin, 0, extent, 1);

        check(placed[i] == expected, "capped track cannot consume an unassigned remainder cell");
        origin += extent;
      }

      children[3].desired = vertical ? CellSize{1, 2} : CellSize{2, 1};

      const auto small = take(grid.arrangeChildren(children, children[3].desired));
      constexpr std::array SMALL_SIZES{1, 1, 0};

      origin = 0;

      for (std::size_t i = 0; i < 3; ++i) {
        const auto expected = vertical ? rect(0, origin, 1, SMALL_SIZES[i]) : rect(origin, 0, SMALL_SIZES[i], 1);

        check(small[i] == expected, "zero quotient leaves remainder in stable track order");
        origin += SMALL_SIZES[i];
      }
    }
  }
}

void allocationFailures() {
  const NodeId id{1, 1, 1};
  GridOptions options;

  options.cells = {{id}};

  Grid grid(options);
  Overlay overlay;

  const std::array children{MeasuredChild{{2, 1}, {}, id}};
  auto sweep = [&](auto &&operation) {
    int rejected{};
    bool complete{};

    for (std::ptrdiff_t budget = 0; budget < 512 && !complete; ++budget) {
      allocation_test::remaining = budget;

      auto result = operation();

      allocation_test::remaining = -1;

      if (result) {
        complete = true;
      } else {
        check(result.error() == TreeError::NO_MEMORY, "allocation failure reports no_memory");
        ++rejected;
      }
    }

    check(complete && rejected > 0, "allocation sweep observes failure and eventual success");
  };

  sweep([&] {
    return grid.measureChildren(children);
  });

  sweep([&] {
    return grid.arrangeChildren(children, {8, 3});
  });

  sweep([&] {
    return overlay.arrangeChildren(children, {8, 3});
  });

  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto node = take(tree->emplace<Grid>(root, options));
  auto replacement = options;

  replacement.column_gap = 2;
  allocation_test::remaining = 0;

  auto rejected = setGridOptions(*tree, node, replacement);

  allocation_test::remaining = -1;
  check(!rejected && rejected.error() == TreeError::NO_MEMORY, "setter allocation failure is observable");
  take(tree->read(node, [](const Grid &value) {
    check(value.options().column_gap == 0, "failed setter preserves old options");
  }));

  sweep([&] {
    return setGridOptions(*tree, node, replacement);
  });
  take(tree->read(node, [](const Grid &value) {
    check(value.options().column_gap == 2, "atomic configuration replacement survives failpoints");
  }));
}
} // namespace

int main() {
  nestedIdentityAndSpans();
  tracksBoundsAndErrors();
  alignmentAndOverlay();
  renderedClippingAndHitOrder();
  stalePlacementsAndPureMeasure();
  verticalSpanAndWeightedTracks();
  cappedSpanRemainders();
  allocationFailures();

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
