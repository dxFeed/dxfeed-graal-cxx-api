// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <toy/api.hpp>

// Check type and symbol identity across a separate header-only TU.
bool headerPeer(toy::Value value, toy::Value (*operation)(toy::Value) noexcept) {
  return operation == &toy::bump && toy::bump(value).number == 42;
}
