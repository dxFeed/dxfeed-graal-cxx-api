// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include "virtual_support.hpp"
#include <portals/virtual_preparation.hpp>

namespace virtualConsumer {
class Dashboard final : public ApplicationScene {
public:
  explicit Dashboard(Application &app, bool pending = false) {
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

    model = take(Model<Ohlc>::create(tree, outer, std::make_shared<const Prices>(!pending)));

    const auto columns = priceColumns();

    table = take(tree.emplace<VirtualTable<Ohlc>>(tablePane, take(model->openRange()), std::span(columns),
                                                  VirtualTableOptions{}, fillHeight));
    chart = take(tree.emplace<Chart<Ohlc>>(chartPane, take(model->openRange()), [](const Ohlc &value) {
      return std::optional{value};
    }));
  }

  bool usesPreparedLayout() const noexcept override {
    return true;
  }

  void stop(Application &) override {
    stopped = true;
  }

  std::expected<void, TreeError> prepare(Application &, CellSize) override {
    auto frame = model->pin();

    if (!frame) {
      return std::unexpected(frame.error());
    }

    // One pin survives every preparation pass and the final render.
    pinned = std::move(*frame);
    pinnedRevision = pinned->revision();
    ++pins;

    return {};
  }

  std::expected<void, TreeError> prepareViews(Application &app, const LayoutSnapshot &layout) override {
    ++viewPasses;

    check(pinned && pinned->revision() == pinnedRevision, "A preparation pass changed the pin");

    auto tableSize = layout.viewport(table);
    auto chartSize = layout.viewport(chart);

    if (!tableSize || !chartSize) {
      return std::unexpected(!tableSize ? tableSize.error() : chartSize.error());
    }

    tableViewport = *tableSize;
    chartViewport = *chartSize;

    auto tableWork = prepareVirtualTable(app.tree(), table, *pinned, layout);

    if (!tableWork) {
      return std::unexpected(tableWork.error());
    }

    // A pending table request invalidates live geometry. The same snapshot still serves its sibling.
    auto chartWork = prepareChart(app.tree(), chart, *pinned, layout);

    if (!chartWork) {
      return std::unexpected(chartWork.error());
    }

    if (*tableWork) {
      tableRequests.push_back(**tableWork);
    }

    if (*chartWork) {
      chartRequests.push_back(**chartWork);
    }

    return {};
  }

  std::unique_ptr<Model<Ohlc>> model;
  Node<Row> body;
  Node<VirtualTable<Ohlc>> table;
  Node<Chart<Ohlc>> chart;
  std::optional<ModelFrame<Ohlc>> pinned;
  std::vector<ModelRequest<Ohlc>> tableRequests;
  std::vector<ModelRequest<Ohlc>> chartRequests;
  CellSize tableViewport;
  CellSize chartViewport;
  std::size_t pins{};
  std::size_t viewPasses{};
  std::uint64_t pinnedRevision{};
  bool stopped{};
};

} // namespace virtualConsumer
