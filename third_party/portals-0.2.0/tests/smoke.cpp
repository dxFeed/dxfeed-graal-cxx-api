// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/version.hpp>

#include <cstdlib>
#include <iostream>

int main() {
  const portals::Version actual = portals::version();

  if (actual.major != EXPECTED_PORTALS_VERSION_MAJOR || actual.minor != EXPECTED_PORTALS_VERSION_MINOR ||
      actual.patch != EXPECTED_PORTALS_VERSION_PATCH) {
    std::cerr << "Expected Portals " << EXPECTED_PORTALS_VERSION_MAJOR << '.' << EXPECTED_PORTALS_VERSION_MINOR << '.'
              << EXPECTED_PORTALS_VERSION_PATCH << ", got " << actual.major << '.' << actual.minor << '.'
              << actual.patch << '\n';

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
