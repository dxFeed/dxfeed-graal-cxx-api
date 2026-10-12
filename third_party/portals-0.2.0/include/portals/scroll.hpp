// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <cstdint>
#include <expected>
#include <portals/widget_types.hpp>

namespace portals {
/** @brief Owning one-dimensional logical position with floor quantization to integral rows.
 * Integral and fractional parts are stored separately, preserving every uint64 index even above 2^53.
 * Fractions use double precision; no epsilon snapping is performed. Saturation discards overscroll.
 * No allocation, callbacks, tree invalidation or I/O occurs. The owner serializes access; views use UI only.
 */
class ScrollState {
public:
  /** @return Integral raster position, in logical rows; text is placed on whole cell rows. */
  std::uint64_t rasterOffset() const noexcept;
  /** @return Fractional logical remainder in [0,1), in logical rows. */
  double fraction() const noexcept;
  /** @brief Replace the unbounded intended position; call clamp when row count is known.
   * @param row Exact integral logical row index.
   * @param fraction Fractional row remainder in [0,1); finite values only.
   * @return Success, or INVALID_ARGUMENT without changing state.
   */
  std::expected<void, TreeError> setOffset(std::uint64_t row, double fraction = 0) noexcept;
  /** @brief Accumulate signed fractional wheel movement, saturating at the first/last row.
   * @param rows Finite signed movement in logical rows; positive moves toward the end.
   * @param count Logical model row count; zero resets the position.
   * @return Whether the raster position changed, or INVALID_ARGUMENT without mutation for nonfinite input.
   */
  std::expected<bool, TreeError> moveBy(double rows, std::uint64_t count) noexcept;
  /** @brief Bound position to the last logical row; an empty model resets both parts.
   * @param count Logical row count, independent of materialized storage.
   */
  void clamp(std::uint64_t count) noexcept;
  /** @brief Reveal an index with minimal integral movement, without changing an already visible position.
   * @param row Target logical index, less than count.
   * @param visibleRows Number of fully visible rows; zero means no movement is possible.
   * @param count Logical model row count.
   * @return Whether raster position changed, or INVALID_ARGUMENT for an out-of-bounds target.
   */
  std::expected<bool, TreeError> ensureVisible(std::uint64_t row, std::uint64_t visibleRows,
                                               std::uint64_t count) noexcept;

private:
  std::uint64_t row_{};
  double fraction_{};
};
} // namespace portals
