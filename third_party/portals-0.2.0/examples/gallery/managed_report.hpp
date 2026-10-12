// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "common.hpp"
#include <portals/managed.hpp>

namespace example {
/** @brief Report typed managed failures after the runner has restored the terminal.
 * @param result Borrowed result with original cause and independent cleanup/restoration errors.
 * @return Zero on success, one on failure; diagnostics go to stderr only on failure.
 * Stream exceptions propagate if enabled by the caller.
 */
inline int report(const std::expected<portals::ApplicationStats, portals::ManagedError> &result) {
  if (result) {
    return 0;
  }

  const auto &error = result.error();

  std::cerr << "Managed operation failed in phase " << static_cast<int>(error.phase) << ": ";
  std::visit(
    [](const auto &cause) {
      using T = std::decay_t<decltype(cause)>;

      if constexpr (std::same_as<T, portals::ApplicationError>) {
        report(std::expected<portals::ApplicationStats, portals::ApplicationError>(std::unexpected(cause)));
      } else if constexpr (std::same_as<T, portals::TreeError>) {
        std::cerr << "TreeError=" << static_cast<int>(cause);
      } else {
        std::cerr << "StyleError=" << static_cast<int>(cause.cause) << " " << cause.message;
      }
    },
    error.cause);

  if (error.cleanup) {
    std::cerr << " cleanup=" << static_cast<int>(*error.cleanup);
  }

  if (error.application) {
    std::cerr << " runner: ";
    report(std::expected<portals::ApplicationStats, portals::ApplicationError>(std::unexpected(*error.application)));
  }

  std::cerr << '\n';

  return 1;
}
} // namespace example
