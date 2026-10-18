// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_support.hpp"

namespace {
using namespace virtualConsumer;

class Dashboard final : public ApplicationScene {
public:
  explicit Dashboard(Application &app) {
    auto &tree = app.tree();
    const auto outer = take(tree.emplace<Column>(app.root(), LinearOptions{.gap = 1, .padding = {1, 1, 1, 1}}));
    const auto title = take(tree.emplace<Label>(outer, text("Immutable prices: table and chart")));

    (void)title;

    SizePolicy fillHeight;
    SizePolicy fillWidth;

    fillHeight.height = {.preferred = 0, .mode = SizeMode::FLEX};
    fillWidth.width = {.preferred = 0, .mode = SizeMode::FLEX};
    body = take(tree.emplace<Row>(outer, LinearOptions{.gap = 2}, fillHeight));

    const auto tablePane = take(tree.emplace<Column>(body, LinearOptions{}, fillWidth));
    const auto chartPane = take(tree.emplace<Column>(body, LinearOptions{.padding = {1, 1, 1, 1}}, fillWidth));

    model = take(Model<Ohlc>::create(tree, outer, std::make_shared<const Prices>()));

    const auto columns = priceColumns();

    table = take(tree.emplace<VirtualTable<Ohlc>>(tablePane, take(model->openRange()), std::span(columns),
                                                  VirtualTableOptions{}, fillHeight));
    chart = take(tree.emplace<Chart<Ohlc>>(chartPane, take(model->openRange()), [](const Ohlc &value) {
      return std::optional{value};
    }));
  }

  std::expected<void, TreeError> prepare(Application &app, CellSize size) override {
    auto frame = model->pin();

    if (!frame) {
      return std::unexpected(frame.error());
    }

    // Keep this arithmetic synchronized with both nested layout containers.
    // Equal FLEX weights give the first sibling any odd remainder cell.
    const CellSize tableSize{std::max(0, (size.width - 4 + 1) / 2), std::max(0, size.height - 4)};
    const CellSize chartSize{std::max(0, size.width - 4 - tableSize.width - 2), std::max(0, tableSize.height - 2)};
    auto tableWork = prepareVirtualTable(app.tree(), table, *frame, tableSize);

    if (!tableWork) {
      return std::unexpected(tableWork.error());
    }

    auto chartWork = prepareChart(app.tree(), chart, *frame, chartSize);

    if (!chartWork) {
      return std::unexpected(chartWork.error());
    }

    // This complete example's immutable provider is already resident.
    if (*tableWork || *chartWork) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    ++pins;

    return {};
  }

  std::unique_ptr<Model<Ohlc>> model;
  Node<Row> body;
  Node<VirtualTable<Ohlc>> table;
  Node<Chart<Ohlc>> chart;
  std::size_t pins{};
};
} // namespace

int main() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  Dashboard dashboard(*app);
  std::size_t settledPins{};

  terminal.checkpoint = [&](std::size_t next, TerminalRead &read) {
    if (next == 0) {
      read.resized = RESIZED_SIZE;
    } else if (next == 1) {
      settledPins = dashboard.pins;
    } else {
      check(dashboard.pins == settledPins, "Idle scene prepared another frame");
    }
  };

  const auto stats = take(app->run(dashboard, terminal));

  check(stats.frames == 2 && dashboard.pins == stats.frames, "Expected one pin per initial/resize frame");
  reportCapture(terminal);

  return EXIT_SUCCESS;
}
