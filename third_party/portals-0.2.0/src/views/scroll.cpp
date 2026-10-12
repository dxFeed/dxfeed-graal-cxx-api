// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <algorithm>
#include <cmath>
#include <limits>
#include <portals/scroll.hpp>

namespace portals {
std::uint64_t ScrollState::rasterOffset() const noexcept {
  return row_;
}

double ScrollState::fraction() const noexcept {
  return fraction_;
}

std::expected<void, TreeError> ScrollState::setOffset(std::uint64_t row, double fraction) noexcept {
  if (!std::isfinite(fraction) || fraction < 0 || fraction >= 1) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  row_ = row;
  fraction_ = fraction;

  return {};
}

void ScrollState::clamp(std::uint64_t count) noexcept {
  const auto last = count == 0 ? 0 : count - 1;

  if (row_ >= last) {
    row_ = last;
    fraction_ = 0;
  }
}

std::expected<bool, TreeError> ScrollState::moveBy(double rows, std::uint64_t count) noexcept {
  if (!std::isfinite(rows)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  const auto previous = row_;
  const auto last = count == 0 ? 0 : count - 1;
  // Conversion is safe only strictly below 2^64. Never convert the stored index to floating point.
  constexpr double UINT64_LIMIT = 0x1p64;
  double whole{};
  const auto partial = std::modf(std::abs(rows), &whole);

  clamp(count);

  if (rows >= 0) {
    if (whole >= UINT64_LIMIT || static_cast<std::uint64_t>(whole) > last - row_) {
      row_ = last;
      fraction_ = 0;
    } else {
      row_ += static_cast<std::uint64_t>(whole);
      fraction_ += partial;

      if (fraction_ >= 1 && row_ < last) {
        ++row_;
        fraction_ -= 1;
      }

      clamp(count);
    }
  } else {
    if (whole >= UINT64_LIMIT || static_cast<std::uint64_t>(whole) > row_) {
      row_ = 0;
      fraction_ = 0;
    } else {
      row_ -= static_cast<std::uint64_t>(whole);
      fraction_ -= partial;

      if (fraction_ < 0) {
        if (row_ != 0) {
          --row_;
          fraction_ += 1;
          // A sub-ULP negative input can round the borrowed remainder to 1. Keep its canonical range.
          fraction_ = std::min(fraction_, std::nextafter(1.0, 0.0));
        } else {
          fraction_ = 0;
        }
      }
    }
  }

  return row_ != previous;
}

std::expected<bool, TreeError> ScrollState::ensureVisible(std::uint64_t row, std::uint64_t visibleRows,
                                                          std::uint64_t count) noexcept {
  if (row >= count) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  const auto previous = row_;

  clamp(count);

  if (visibleRows != 0) {
    if (row < row_) {
      row_ = row;
      fraction_ = 0;
    } else if (row - row_ >= visibleRows) {
      row_ = row - visibleRows + 1;
      fraction_ = 0;
    }
  }

  return row_ != previous;
}
} // namespace portals
