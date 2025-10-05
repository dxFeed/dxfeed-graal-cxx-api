// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0


#include <config/config.hpp>
#include <iostream>

using namespace org::ttldtor::config;

int main() {
  Config config{};

  config.addSource(IniSource::fromString(R"("
user=demo
password=demo
  ")"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  // set MY_PREFIX_user=test
  config.addSource(EnvSource("MY_PREFIX_"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  return 0;
}