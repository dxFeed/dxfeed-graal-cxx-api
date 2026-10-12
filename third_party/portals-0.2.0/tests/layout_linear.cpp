// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <portals/layout.hpp>
#include <type_traits>
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

using namespace portals;

namespace {
int failures{};

void check(bool ok, const char *message) {
  if (!ok) {
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

struct Measuring : SizedWidget {
  CellSize desired;
  int calls{};

  Measuring(CellSize size, SizePolicy policy = {}) : SizedWidget(policy), desired(size) {
  }

  std::expected<CellSize, TreeError> measure(CellSize) override {
    ++calls;

    return desired;
  }
};

void oddFlex() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto row = take(tree->emplace<Row>(root));
  SizePolicy flex;

  flex.width.mode = SizeMode::FLEX;

  auto first = take(tree->emplace<Measuring>(row, CellSize{0, 1}, flex));
  auto second = take(tree->emplace<Measuring>(row, CellSize{0, 1}, flex));
  auto viewport = take(CellRect::from({}, {11, 2}));
  std::expected<void, TreeError> result;

  take(tree->frame([&] {
    take(tree->arrange(root, viewport, viewport));

    result = layoutTree(*tree, row, viewport, viewport);
  }));
  check(result.has_value(), "layout succeeds");
  check(take(tree->inspect(first)).geometry.local.size().width == 6, "first gets odd remainder");
  check(take(tree->inspect(second)).geometry.local.origin().x == 6, "second follows first");
  check(take(tree->inspect(second)).geometry.local.size().width == 5, "second gets floor share");
}

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());

  std::expected<void, TreeError> run(CellSize size) {
    auto viewport = take(CellRect::from({}, size));

    take(tree->invalidate(root, Invalidation::LAYOUT));

    std::expected<void, TreeError> result;

    take(tree->frame([&] {
      result = layoutTree(*tree, root, viewport, viewport);
    }));

    return result;
  }
};

SizePolicy widthPolicy(SizeMode mode, int minimum, int preferred, int maximum) {
  SizePolicy result;

  result.width = {minimum, preferred, maximum, mode, 1};

  return result;
}

void cappedAndMixed() {
  Fixture f;
  LinearOptions options;

  options.gap = 1;
  options.padding = {1, 1, 1, 1};

  auto row = take(f.tree->emplace<Row>(f.root, options));
  auto fixed = take(f.tree->emplace<Measuring>(row, CellSize{9, 1}, widthPolicy(SizeMode::FIXED, 0, 3, 10)));
  auto automatic = take(f.tree->emplace<Measuring>(row, CellSize{2, 1}));
  auto flex1 = take(f.tree->emplace<Measuring>(row, CellSize{0, 1}, widthPolicy(SizeMode::FLEX, 1, 1, 2)));
  auto flex2 = take(f.tree->emplace<Measuring>(row, CellSize{0, 1}, widthPolicy(SizeMode::FLEX, 1, 1, 20)));

  take(f.run({20, 5}));
  check(take(f.tree->inspect(fixed)).geometry.local == take(CellRect::from({1, 1}, {3, 3})), "fixed and padding");
  check(take(f.tree->inspect(automatic)).geometry.local.size().width == 2, "auto uses actual intrinsic measure");
  check(take(f.tree->inspect(flex1)).geometry.local.size().width == 2, "first flex capped");
  check(take(f.tree->inspect(flex2)).geometry.local.size().width == 8, "cap remainder redistributes");
  take(setSizePolicy(*f.tree, flex2, widthPolicy(SizeMode::FLEX, 1, 1, 3)));
  check(!take(f.tree->inspect(fixed)).geometry.valid, "policy mutation invalidates geometry");
  take(f.run({20, 5}));
  check(take(f.tree->inspect(flex2)).geometry.local.size().width == 3, "all maxima permit unused cells");
  take(f.tree->setEnabled(row, false));
  take(f.run({20, 5}));
  check(take(f.tree->inspect(flex1)).geometry.local.size().width == 2, "disabled keeps layout");
}

void underflowAndCenter() {
  Fixture f;
  LinearOptions options;

  options.gap = 1;

  auto row = take(f.tree->emplace<Row>(f.root, options));
  auto first =
    take(f.tree->emplace<Measuring>(row, CellSize{8, 2}, widthPolicy(SizeMode::AUTOMATIC, 4, INTRINSIC_EXTENT, 9)));

  auto second =
    take(f.tree->emplace<Measuring>(row, CellSize{8, 2}, widthPolicy(SizeMode::AUTOMATIC, 4, INTRINSIC_EXTENT, 9)));

  take(f.run({5, 2}));

  auto a = take(f.tree->inspect(first)).geometry;
  auto b = take(f.tree->inspect(second)).geometry;

  check(a.local.size().width == 4 && b.local.size().width == 4, "underflow preserves minima");
  check(b.local.origin().x == 5 && b.clip.empty(), "overflow clipped at ancestor");
  take(f.tree->setVisible(second, false));
  options.main_align = LayoutAlign::CENTER;
  options.cross_align = LayoutAlign::CENTER;
  take(setLinearOptions(*f.tree, row, options));
  take(setSizePolicy(*f.tree, first, widthPolicy(SizeMode::FIXED, 0, 2, 9)));

  for (int width : {8, 9}) {
    take(f.run({width, 5}));

    auto rect = take(f.tree->inspect(first)).geometry.local;

    check(rect.origin().x == (width - 2) / 2 && rect.origin().y == 1, "center floor before; remainder after");
  }

  check(!take(f.tree->inspect(second)).geometry.valid, "hidden subtree remains invalid");

  int calls{};

  take(f.tree->read(second, [&](const Measuring &widget) {
    calls = widget.calls;
  }));
  take(f.run({9, 5}));
  take(f.tree->read(second, [&](const Measuring &widget) {
    check(widget.calls == calls, "hidden widget never measured");
  }));
}

void nestingAndSnapshots() {
  Fixture f;
  auto column = take(f.tree->emplace<Column>(f.root));
  SizePolicy tall;

  tall.height.mode = SizeMode::FLEX;

  auto row = take(f.tree->emplace<Row>(column, LinearOptions{}, tall));
  auto label = take(f.tree->emplace<Measuring>(row, CellSize{3, 1}));
  auto footer = take(f.tree->emplace<Measuring>(column, CellSize{2, 1}));

  take(f.run({10, 6}));

  auto saved = take(f.tree->inspect(label));

  check(take(f.tree->inspect(row)).geometry.local.size() == CellSize{10, 5}, "nested row receives flex height");
  check(take(f.tree->inspect(footer)).geometry.local.origin().y == 5, "column positions following child");
  take(f.run({12, 7}));
  check(saved.geometry.local.size() == CellSize{3, 5}, "snapshot owns previous geometry");
  check(take(f.tree->inspect(label)).geometry.local.size() == CellSize{3, 6}, "nested geometry updated");
}

struct Recursive : SizedWidget {
  WidgetTree &tree;
  Node<Widget> root;
  bool guarded{};

  Recursive(WidgetTree &owner, Node<Widget> node) : tree(owner), root(node) {
  }

  std::expected<CellSize, TreeError> measure(CellSize) override {
    auto nested = layoutTree(tree, root, {}, {});
    auto mutation = tree.setEnabled(root, false);

    guarded =
      !nested && nested.error() == TreeError::WOULD_BLOCK && !mutation && mutation.error() == TreeError::WOULD_BLOCK;

    return CellSize{1, 1};
  }
};

void errorsAndGuard() {
  Fixture f;
  auto row = take(f.tree->emplace<Row>(f.root));
  auto recursive = take(f.tree->emplace<Recursive>(row, *f.tree, f.root));

  take(f.run({4, 2}));
  take(f.tree->read(recursive, [&](const Recursive &widget) {
    check(widget.guarded, "reentry and hook mutation rejected");
  }));

  auto bad = widthPolicy(SizeMode::FIXED, 0, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
  auto huge = take(f.tree->emplace<Measuring>(row, CellSize{1, 1}, bad));
  auto failure = f.run({4, 2});

  check(!failure && failure.error() == TreeError::INVALID_GEOMETRY, "aggregate geometry overflow explicit");
  check(!take(f.tree->inspect(recursive)).geometry.valid && !take(f.tree->inspect(huge)).geometry.valid,
        "failed invalidated pass cannot expose stale rectangles");
  bad.width.minimum = -1;

  auto rejected = setSizePolicy(*f.tree, huge, bad);

  check(!rejected && rejected.error() == TreeError::INVALID_ARGUMENT, "invalid policy rejected");
}

void geometryProperties() {
  Row row;

  for (int count = 1; count <= 6; ++count) {
    for (int available = 0; available <= 31; ++available) {
      std::vector<MeasuredChild> children(static_cast<std::size_t>(count));
      int cap_sum{};

      for (int i = 0; i < count; ++i) {
        children[static_cast<std::size_t>(i)].policy = widthPolicy(SizeMode::FLEX, 0, 0, i + 2);
        cap_sum += i + 2;
      }

      auto rects = take(row.arrangeChildren(children, {available, 3}));
      int sum{};

      for (int i = 0; i < count; ++i) {
        auto rect = rects[static_cast<std::size_t>(i)];

        check(rect.origin().x == sum && rect.size().width >= 0 && rect.size().width <= i + 2,
              "bounded contiguous flex");
        sum += rect.size().width;
      }

      check(sum == std::min(available, cap_sum), "flex fills exactly until all caps");
    }
  }
}

void weightedAndCheckedTranslation() {
  Row row;
  std::vector<MeasuredChild> children(2);

  children[0].policy = widthPolicy(SizeMode::FLEX, 0, 0, 20);
  children[1].policy = widthPolicy(SizeMode::FLEX, 0, 0, 20);
  children[1].policy.width.weight = 2;

  auto rects = take(row.arrangeChildren(children, {10, 2}));

  check(rects[0].size().width == 4 && rects[1].size().width == 6, "weighted quotient with stable remainder");

  Fixture f;
  auto container = take(f.tree->emplace<Row>(f.root));
  auto child = take(f.tree->emplace<Measuring>(container, CellSize{6, 1}, widthPolicy(SizeMode::FIXED, 0, 6, 6)));
  auto viewport = take(CellRect::from({std::numeric_limits<std::int32_t>::max() - 5, 0}, {5, 1}));
  std::expected<void, TreeError> result;

  take(f.tree->invalidate(f.root, Invalidation::LAYOUT));
  take(f.tree->frame([&] {
    result = layoutTree(*f.tree, f.root, viewport, viewport);
  }));
  check(!result && result.error() == TreeError::INVALID_GEOMETRY, "surface translation overflow checked");
  check(!take(f.tree->inspect(child)).geometry.valid, "failed child has invalid geometry");
}

struct IdentityRow : Row {
  mutable std::vector<NodeId> ids;

  std::expected<CellSize, TreeError> measureChildren(std::span<const MeasuredChild> children) const override {
    ids.clear();

    for (const auto &child : children) {
      ids.push_back(child.node);
    }

    return Row::measureChildren(children);
  }
};

void identitiesWithHiddenSiblings() {
  Fixture f;
  auto row = take(f.tree->emplace<IdentityRow>(f.root));
  auto first = take(f.tree->emplace<Measuring>(row, CellSize{1, 1}));
  auto middle = take(f.tree->emplace<Measuring>(row, CellSize{1, 1}));
  auto last = take(f.tree->emplace<Measuring>(row, CellSize{1, 1}));

  take(f.run({10, 3}));
  take(f.tree->read(row, [&](const IdentityRow &widget) {
    check(widget.ids == std::vector<NodeId>{first.id(), middle.id(), last.id()}, "all stable IDs reach pure solver");
  }));
  take(f.tree->setVisible(middle, false));
  take(f.run({10, 3}));
  take(f.tree->read(row, [&](const IdentityRow &widget) {
    check(widget.ids == std::vector<NodeId>{first.id(), last.id()}, "hidden sibling does not rebind remaining child");
  }));
}

void allocationFailures() {
  Fixture f;
  auto row = take(f.tree->emplace<Row>(f.root));

  take(f.tree->emplace<Measuring>(row, CellSize{2, 1}));

  auto viewport = take(CellRect::from({}, {8, 3}));
  bool completed{};
  int observed{};

  for (std::ptrdiff_t budget = 0; budget < 256 && !completed; ++budget) {
    take(f.tree->invalidate(f.root, Invalidation::LAYOUT));

    std::expected<void, TreeError> result;

    take(f.tree->frame([&] {
      allocation_test::remaining = budget;
      result = layoutTree(*f.tree, f.root, viewport, viewport);
      allocation_test::remaining = -1;
    }));

    if (result) {
      completed = true;
    } else {
      check(result.error() == TreeError::NO_MEMORY, "allocation error remains observable");
      ++observed;
    }
  }

  check(completed && observed > 0, "real allocation failpoints eventually succeed");
}
} // namespace

int main() {
  oddFlex();
  cappedAndMixed();
  underflowAndCenter();
  nestingAndSnapshots();
  errorsAndGuard();
  geometryProperties();
  weightedAndCheckedTranslation();
  identitiesWithHiddenSiblings();
  allocationFailures();

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}