// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/version.hpp>

#include <iostream>

int main() {
  const portals::Version library_version = portals::version();

  std::cout << "Portals " << library_version.major << '.' << library_version.minor << '.' << library_version.patch
            << '\n';

  return 0;
}
