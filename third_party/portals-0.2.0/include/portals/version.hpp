// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

/** @brief Public Portals API. */
namespace portals {

/** @brief Version components of the linked Portals library, owned by the caller. */
struct Version {
  unsigned major; ///< Major release component.
  unsigned minor; ///< Minor release component.
  unsigned patch; ///< Patch release component.
};

/**
 * @brief Return the version of the linked library by value.
 * @return Independent version components; no resources need to be released.
 * @note Callable from any thread, including concurrently. No allocation, I/O,
 *       exceptions, or error states. Returned values are never invalidated.
 *       This is release metadata, not an API/ABI compatibility guarantee.
 */
Version version() noexcept;

} // namespace portals
