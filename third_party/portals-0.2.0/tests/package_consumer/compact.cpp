// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/aggregate_columns.hpp>
#include <portals/input_match.hpp>
#include <portals/text_format.hpp>

#include <array>
#include <string>

struct Quote {
  double bid;
  double ask;
};

int main() {
  using namespace portals;
  static_assert(keyPressed(InputEvent{KeyEvent{.key = InputKey::F2}}, {InputKey::F1, InputKey::F2}));
  auto columns =
    aggregateColumns<Quote>(std::array{ColumnCaption{"Bid", 9}, ColumnCaption{"Ask", 9}}, cellFormat<double>("{:.2f}"));

  if (!columns) {
    return 1;
  }
  const auto named = tableColumns(tableColumn("Ask", &Quote::ask, cellFormat<double>("{:.2f}"), 9));
  const Quote quote{10.25, 11.5};

  return named && (*columns)[0].format(quote).bytes() == "10.25" && (*columns)[1].format(quote).bytes() == "11.50" &&
             (*named)[0].format(quote).bytes() == "11.50"
           ? 0
           : 1;
}
