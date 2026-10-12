// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <portals/widget_types.hpp>

namespace portals {
/** @brief Synchronous clipped cell drawing inside a retained widget's paint hook.
 * The context and optional brush are borrowed through the enclosing frame. Commands use
 * widget-local coordinates; the frame sink applies surface translation and ancestor clipping.
 * No I/O, allocation, model access or invalidation. Discard the frame's commands on error.
 */
class Canvas {
public:
  /** @brief Borrow a sink and optional one-cell glyph for this frame.
   * @param context UI-owned command sink, alive through all drawing calls.
   * @param clip Half-open local cell clip, intersected with every emitted command.
   * @param brush Optional one-cell, one-grapheme brush alive and unchanged through frame rendering;
   * null draws styled spaces, useful with a background color. Invalid brushes fail on drawing.
   */
  Canvas(PaintContext &context, CellRect clip, const GraphemeText *brush = nullptr) noexcept;
  /** @brief Draw a cell if it is inside the clip.
   * @param position Widget-local cell coordinate.
   * @param style Owning cell paint value.
   * @return Success for a clipped-out point, INVALID_ARGUMENT for an invalid brush, or the sink error.
   */
  std::expected<void, TreeError> point(CellPoint position, CellStyle style = {});
  /** @brief Rasterize an inclusive DDA line, rounding ties away from zero, then clip.
   * Only the clip's dominant-axis coordinates are visited, even for INT32_MIN/MAX endpoints.
   * @param first Starting cell coordinate; negative and offscreen values are allowed.
   * @param last Ending cell coordinate, included when visible.
   * @param style Owning paint value shared by all rasterized cells.
   * @return Success, invalid brush, or the first sink error; earlier commands may already be submitted.
   */
  std::expected<void, TreeError> line(CellPoint first, CellPoint last, CellStyle style = {});

private:
  PaintContext &context_;
  CellRect clip_;
  const GraphemeText *brush_;
};
} // namespace portals
