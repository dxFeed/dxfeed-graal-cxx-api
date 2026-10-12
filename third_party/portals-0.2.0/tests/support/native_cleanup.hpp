// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <ostream>
#include <string>
#include <vector>

namespace native {
struct Options {
  std::vector<std::string> child;
  std::string title;
  int deadlineMs{15000};
  int quitMs{4500};
  int expectedExit{};
  bool drive{};
};

bool supervise(const Options &options, std::ostream &log);
} // namespace native
