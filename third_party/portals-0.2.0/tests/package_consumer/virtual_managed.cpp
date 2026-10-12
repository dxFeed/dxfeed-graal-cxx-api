// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_support.hpp"
#include <portals/managed.hpp>

using namespace virtualConsumer;

int main() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto outer = take(scene->emplace<Column>(scene->root(), LinearOptions{.gap = 1, .padding = {1, 1, 1, 1}}));
  const auto title = take(scene->emplace<Label>(outer, text("Immutable prices: table and chart")));

  (void)title;

  SizePolicy fillHeight;
  SizePolicy fillWidth;

  fillHeight.height = {.preferred = 0, .mode = SizeMode::FLEX};
  fillWidth.width = {.preferred = 0, .mode = SizeMode::FLEX};

  const auto body = take(scene->emplace<Row>(outer, LinearOptions{.gap = 2}, fillHeight));
  const auto tablePane = take(scene->emplace<Column>(body, LinearOptions{}, fillWidth));
  const auto chartPane = take(scene->emplace<Column>(body, LinearOptions{.padding = {1, 1, 1, 1}}, fillWidth));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  const auto columns = priceColumns();
  const auto table = take(scene->virtualTable(tablePane, model, columns, {}, fillHeight));
  const auto chart = take(scene->chart(chartPane, model, [](const Ohlc &value) {
    return std::optional{value};
  }));

  (void)table;
  (void)chart;

  terminal.checkpoint = [](std::size_t next, TerminalRead &read) {
    if (next == 0) {
      read.resized = RESIZED_SIZE;
    }
  };

  const auto stats = take(scene->run(terminal));

  check(stats.frames == 2, "Managed scene redrew while idle");
  check(!model.publisher() && model.publisher().error() == TreeError::CLOSED, "Model survived scene cleanup");
  reportCapture(terminal);

  return EXIT_SUCCESS;
}
