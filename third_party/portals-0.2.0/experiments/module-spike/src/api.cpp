// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <toy/api.hpp>

#include <limits>

toy::Value toy::bump(Value value) noexcept {
  if (value.number != std::numeric_limits<int>::max()) {
    ++value.number;
  }

  return value;
}

long long toy::sum(std::span<const int> values) noexcept {
  long long result = 0;

  for (int value : values) {
    result += value;
  }

  return result;
}
