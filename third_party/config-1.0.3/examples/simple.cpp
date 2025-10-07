// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <config/config.hpp>
#include <iostream>

using namespace org::ttldtor::config;

// set MY_PREFIX_user=test

int main() {
  Config config{};

  config.addSource(IniSource::fromString(R"("
user=demo
password=demo
  ")"));

  for (const auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  // user = demo
  // password = demo

  config.addSource(EnvSource("MY_PREFIX_"));

  for (const auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  // user = test
  // password = demo

  return 0;
}