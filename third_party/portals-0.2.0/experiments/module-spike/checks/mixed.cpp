// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include "standard.hpp"

#include <toy/api.hpp>

import toy;

#include "values.hpp"

bool headerPeer(toy::Value value, toy::Value (*operation)(toy::Value) noexcept);

int main() {
  if (!headerPeer({41}, &toy::bump)) {
    return 4;
  }

  return checkValues();
}
