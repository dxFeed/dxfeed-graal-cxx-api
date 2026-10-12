// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/version.hpp>

namespace portals {

Version version() noexcept {
  return {PORTALS_VERSION_MAJOR, PORTALS_VERSION_MINOR, PORTALS_VERSION_PATCH};
}

} // namespace portals
