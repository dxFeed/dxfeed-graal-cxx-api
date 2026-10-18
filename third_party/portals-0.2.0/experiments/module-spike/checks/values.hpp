// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "standard.hpp"

// Include this after the selected header/import boundary. No toy header here.
inline int checkValues() {
  const std::array values{4, -7, 12};
  const auto original = toy::Value{41};
  const auto copy = toy::identity(original);

  if (toy::bump(copy).number != 42 || original.number != 41) {
    return 1;
  }

  if (toy::sum(values) != 9 || toy::sum({}) != 0) {
    return 2;
  }

  const int maximum = std::numeric_limits<int>::max();
  const std::array wide{maximum, maximum, -maximum};

  if (toy::sum(wide) != static_cast<long long>(maximum) || toy::bump({maximum}).number != maximum) {
    return 3;
  }

  return 0;
}
