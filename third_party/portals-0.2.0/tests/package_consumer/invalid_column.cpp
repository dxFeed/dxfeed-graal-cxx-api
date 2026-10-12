// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/table_column.hpp>

struct Row {
  int value;
};

void invalidColumn() {
  portals::TableColumnOptions options{.id = {1, 1}, .header = portals::GraphemeText::from("Value").value()};

#ifdef PORTALS_INVALID_PROJECTION
  auto column = portals::tableColumn<Row>(
    options,
    [](Row &) {
      return 1;
    },
    [](int) {
      return portals::GraphemeText::from("value").value();
    });
#else
  auto column = portals::tableColumn<Row>(options, &Row::value, [](int) {
    return 1;
  });
#endif

  (void)column;
}
