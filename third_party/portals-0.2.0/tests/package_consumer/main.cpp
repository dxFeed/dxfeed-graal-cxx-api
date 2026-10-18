// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/model.hpp>
#include <portals/version.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
using namespace portals;

template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    std::cerr << "Package consumer operation failed\n";
    std::exit(EXIT_FAILURE);
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

struct Numbers final : ModelSnapshot<int> {
  std::array<ModelRow<int>, 2> values;

  Numbers(std::uint64_t revision, int first, int second)
      : ModelSnapshot(revision, 2), values{{{{1, 1}, first}, {{2, 1}, second}}} {
  }

  ModelRows<int> read(RowRange range) const override {
    if (range.first > values.size() || range.count > values.size() - range.first) {
      return {};
    }

    return {RangeState::READY, std::span(values).subspan(range.first, range.count), {}};
  }
};

struct TotalWidget final : Widget {
  GraphemeText text = take(GraphemeText::from("0"));
  CellRect bounds;

  void prepare(const ModelRange<int> &range) {
    int total = std::accumulate(range.rows().begin(), range.rows().end(), 0, [](int sum, const ModelRow<int> &row) {
      return sum + row.value;
    });

    text = take(GraphemeText::from(std::to_string(total)));
  }

  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return CellSize{available.width, 1};
  }

  std::expected<void, TreeError> arrange(CellRect local) override {
    bounds = local;

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    RenderCommand command = TextCommand{bounds.origin(), text, bounds};

    return context.submit(std::span(&command, 1));
  }
};

struct Recorder final : PaintContext {
  std::vector<RenderCommand> commands;

  std::expected<void, TreeError> submit(std::span<const RenderCommand> values) override {
    for (const auto &value : values) {
      commands.push_back(value);
    }

    return {};
  }
};
} // namespace

int main() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto widget = take(tree->emplace<TotalWidget>(root));
  auto model = take(Model<int>::create(*tree, root, std::make_shared<const Numbers>(1, 3, 7)));
  auto oldFrame = take(model->pin());

  take(model->publisher().publish(std::make_shared<const Numbers>(2, 20, 22)));
  take(scheduler->dispatch());

  auto frame = take(model->pin());
  auto rows = take(frame.read({0, 2}));

  check(frame.dataRevision() == 2 && oldFrame.dataRevision() == 1, "Model publication lost revision isolation");
  check(take(oldFrame.read({0, 2})).rows()[0].value == 3, "Old model frame changed");
  take(tree->edit(widget, [&](TotalWidget &value) {
    value.prepare(rows);
  }));
  check(take(tree->measure(widget, {8, 1})) == CellSize{8, 1}, "Custom measurement failed");

  auto viewport = take(CellRect::from({}, {8, 1}));

  take(tree->arrange(root, viewport, viewport));
  take(tree->arrange(widget, viewport, viewport));
  take(tree->frame([&] {
    Recorder sink;

    take(tree->paint(widget, sink));

    auto rendered = take(renderFullFrame(viewport.size(), sink.commands));

    check(take(rendered.at({0, 0})).glyph == "4" && take(rendered.at({1, 0})).glyph == "2",
          "Custom widget did not paint the published model total");
  }));

  auto linkedVersion = version();

  check(linkedVersion.major == EXPECTED_PORTALS_VERSION_MAJOR &&
          linkedVersion.minor == EXPECTED_PORTALS_VERSION_MINOR &&
          linkedVersion.patch == EXPECTED_PORTALS_VERSION_PATCH,
        "Wrong package version linked");
  std::cout << "Installed Portals: model revision 2, custom widget total 42\n";

  return EXIT_SUCCESS;
}
