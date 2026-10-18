// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/legacy.hpp>

/** @brief Historical names for the minimal MarketDepthModelSample streaming API.
 * New consumers should include portals/legacy.hpp and name portals::legacy directly.
 * This header does not restore the complete Portals 0.1.x API or its bit utilities.
 */
namespace org::ttldtor {

using VirtualTerminal = ::portals::legacy::VirtualTerminal; ///< Historical output-mode helper.

namespace portals = ::portals::legacy; ///< Historical cursor-string namespace.

} // namespace org::ttldtor
