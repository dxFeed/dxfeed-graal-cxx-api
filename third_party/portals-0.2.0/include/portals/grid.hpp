// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/layout.hpp>

namespace portals {
/** @brief Owning explicit placement, keyed by full identity rather than visible sibling index. */
struct GridCell {
  NodeId node;                ///< Owning identity; hidden/removed entries never bind a replacement node.
  std::size_t row{};          ///< Zero-based first row.
  std::size_t column{};       ///< Zero-based first column.
  std::size_t row_span{1};    ///< Positive span wholly within the explicit rows.
  std::size_t column_span{1}; ///< Positive span wholly within the explicit columns.
  LayoutAlign horizontal{LayoutAlign::STRETCH}; ///< Child alignment within its columns.
  LayoutAlign vertical{LayoutAlign::STRETCH};   ///< Child alignment within its rows.
};

/** @brief Owning sparse grid configuration; no implicit tracks or automatic placement. */
struct GridOptions {
  std::vector<AxisSize> columns{AxisSize{}};              ///< Explicit horizontal tracks using shared size policies.
  std::vector<AxisSize> rows{AxisSize{}};                 ///< Explicit vertical tracks using shared size policies.
  std::int32_t column_gap{};                              ///< Nonnegative gap, included inside a spanning cell.
  std::int32_t row_gap{};                                 ///< Nonnegative gap, included inside a spanning cell.
  LayoutPadding padding;                                  ///< Insets retained during shortage.
  std::vector<GridCell> cells = std::vector<GridCell>(0); ///< Unique identities; overlap is allowed in sibling order.
  std::size_t max_tracks_per_axis{4096};                  ///< Positive work budget for each explicit axis.
  std::size_t max_placements{4096};                       ///< Positive work budget, including inactive placements.
};

/** @brief UI-owned Grid implementing the existing pure Layout contract.
 * Every visible child needs a placement. All spans are checked, including hidden/stale entries.
 * Later siblings paint/hit above earlier siblings; placement order does not change stacking.
 * Intrinsic tracks absorb child demand, then Row's allocation rules apply independently on each axis.
 * Constructors own settings; invalid settings are reported by Layout methods. Layout errors are
 * INVALID_ARGUMENT, NODE_LIMIT, INVALID_GEOMETRY or NO_MEMORY. No I/O, retained spans or tree traversal.
 */
class Grid : public SizedWidget, public Layout {
public:
  /** @brief Own settings and size policies.
   * @param options Tracks, sparse placements and limits copied into independent storage.
   * @param policy Size policy of this grid in its parent.
   * @throws std::bad_alloc If copying the configuration cannot allocate storage.
   */
  explicit Grid(const GridOptions &options = {}, SizePolicy policy = {});
  /** @brief Owning UI-thread copy; allocation may throw bad_alloc.
   * @return An independent copy of the complete sparse configuration.
   * @throws std::bad_alloc If the owning copy cannot be allocated.
   */
  GridOptions options() const;
  /** @brief Checked inset bounds.
   * @param available Container extent offered by its parent, in cells.
   * @return Normalized inner extent after padding, or invalid settings/coordinate overflow.
   */
  std::expected<CellSize, TreeError> childAvailable(CellSize available) const override;
  /** @brief Aggregate tracks.
   * @param children Visible child measurements and policies, borrowed only for this calculation.
   * @return Aggregate intrinsic extent including gaps/padding, or a validation, geometry or allocation error.
   */
  std::expected<CellSize, TreeError> measureChildren(std::span<const MeasuredChild> children) const override;
  /** @brief Child-order local rectangles.
   * @param children Visible child measurements, policies and identities in sibling order.
   * @param available Container-local extent in terminal cells.
   * @return Owning rectangles in child order, or INVALID_ARGUMENT, NODE_LIMIT, INVALID_GEOMETRY or NO_MEMORY as
   * applicable.
   */
  std::expected<std::vector<CellRect>, TreeError> arrangeChildren(std::span<const MeasuredChild> children,
                                                                  CellSize available) const override;

private:
  friend std::expected<void, TreeError> setGridOptions(WidgetTree &, Node<Grid>, const GridOptions &);
  GridOptions options_;
};

/** @brief Owning overlay alignment and insets; all children share the content rectangle. */
struct OverlayOptions {
  LayoutPadding padding;                        ///< Insets retained during shortage.
  LayoutAlign horizontal{LayoutAlign::STRETCH}; ///< Alignment of each child across columns.
  LayoutAlign vertical{LayoutAlign::STRETCH};   ///< Alignment of each child across rows.
};

/** @brief UI-owned overlay using the existing traversal, geometry and sibling stacking.
 * Later siblings are above earlier siblings for normal paint order and EventRouter hit testing.
 * Layout methods are pure and follow Layout's borrowed-span contract. Invalid settings/policies
 * return INVALID_ARGUMENT, checked overflow INVALID_GEOMETRY, allocation failure NO_MEMORY.
 */
class Overlay : public SizedWidget, public Layout {
public:
  /** @brief Own settings and size policies.
   * @param options Owning padding and alignment applied independently to every child.
   * @param policy Size policy of this overlay in its parent.
   */
  explicit Overlay(OverlayOptions options = {}, SizePolicy policy = {});
  /** @brief Owning UI-thread copy.
   * @return A value copy of the current padding and alignment.
   */
  OverlayOptions options() const;
  /** @brief Checked inset bounds.
   * @param available Container extent offered by its parent, in cells.
   * @return Normalized inner extent after padding, or invalid settings/coordinate overflow.
   */
  std::expected<CellSize, TreeError> childAvailable(CellSize available) const override;
  /** @brief Maximum child extent on each axis.
   * @param children Visible child measurements and policies, borrowed only for this calculation.
   * @return Aggregate intrinsic extent including gaps/padding, or a validation, geometry or allocation error.
   */
  std::expected<CellSize, TreeError> measureChildren(std::span<const MeasuredChild> children) const override;
  /** @brief Child-order local rectangles.
   * @param children Visible child measurements, policies and identities in sibling order.
   * @param available Container-local extent in terminal cells.
   * @return Owning rectangles in child order, or INVALID_ARGUMENT, NODE_LIMIT, INVALID_GEOMETRY or NO_MEMORY as
   * applicable.
   */
  std::expected<std::vector<CellRect>, TreeError> arrangeChildren(std::span<const MeasuredChild> children,
                                                                  CellSize available) const override;

private:
  friend std::expected<void, TreeError> setOverlayOptions(WidgetTree &, Node<Overlay>, OverlayOptions);
  OverlayOptions options_;
};

/** @brief Validate and atomically copy settings on the UI owner between frames, then invalidate layout.
 * Tree errors propagate; invalid configuration leaves the previous settings unchanged.
 * Input is borrowed only for this call. Allocation failure returns NO_MEMORY.
 * @param tree Owning tree, borrowed for the UI-only update.
 * @param node Checked observer of the grid being updated.
 * @param options Complete replacement configuration; borrowed only while making an independent copy.
 * @return Success after replacement and layout invalidation, or a validation/access/allocation error.
 */
std::expected<void, TreeError> setGridOptions(WidgetTree &tree, Node<Grid> node, const GridOptions &options);
/** @brief Validate and replace settings on the UI owner between frames, then invalidate layout.
 * @param tree Owning tree, borrowed for the UI-only update.
 * @param node Checked observer of the overlay being updated.
 * @param options Replacement padding and alignment, copied by value.
 * @return Success after replacement and layout invalidation, or a validation/access error with old settings retained.
 */
std::expected<void, TreeError> setOverlayOptions(WidgetTree &tree, Node<Overlay> node, OverlayOptions options);
} // namespace portals
