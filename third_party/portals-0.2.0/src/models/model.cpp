// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/model.hpp>

namespace portals::detail {
std::expected<std::uint64_t, TreeError> modelSession() {
  static std::atomic<std::uint64_t> sequence{};
  auto current = sequence.load();

  // Reserve a process-wide session without wrapping: old consumer IDs may survive their
  // source through pinned frames. CAS refreshes current after contention, including spurious failure.
  for (;;) {
    if (current == std::numeric_limits<std::uint64_t>::max()) {
      return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
    }

    if (sequence.compare_exchange_weak(current, current + 1)) {
      return current + 1;
    }
  }
}
} // namespace portals::detail
