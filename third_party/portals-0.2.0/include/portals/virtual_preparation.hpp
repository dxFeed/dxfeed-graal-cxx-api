// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/chart.hpp>
#include <portals/layout_snapshot.hpp>
#include <portals/virtual_table.hpp>

#include <concepts>

namespace portals {
/** @brief Prepare a list using its captured local layout extent.
 * @tparam T Immutable row type satisfying VirtualList's provider and delegate contract.
 * @tparam Geometry Deduced LayoutSnapshot; deduction preserves existing braced CellSize calls.
 * @param tree Borrowed UI owner; call outside its frame.
 * @param node Checked observing list handle from this tree.
 * @param frame Shared model pin retained across all sibling preparations and layout retries.
 * @param layout Owning geometry captured for this preparation pass; never consults live geometry.
 * @return New optional provider request, a snapshot identity error, or the original preparation error.
 * @note Hidden ancestors retain the existing cancellation behavior. Caller dispatches requests;
 * do not dispatch the scheduler or repin between sibling preparations.
 */
template <class T, class Geometry>
  requires std::same_as<Geometry, LayoutSnapshot>
std::expected<std::optional<ModelRequest<T>>, TreeError>
prepareVirtualList(WidgetTree &tree, Node<VirtualList<T>> node, const ModelFrame<T> &frame, const Geometry &layout) {
  auto viewport = layout.viewport(node);

  if (!viewport) {
    return std::unexpected(viewport.error());
  }

  return prepareVirtualList(tree, node, frame, *viewport);
}

/** @brief Prepare a table using its captured local layout extent, including its header.
 * @tparam T Immutable row type satisfying VirtualTable's provider and column contract.
 * @tparam Geometry Deduced LayoutSnapshot; deduction preserves existing braced CellSize calls.
 * @param tree Borrowed UI owner; call outside its frame.
 * @param node Checked observing table handle from this tree.
 * @param frame Shared model pin retained across all sibling preparations and layout retries.
 * @param layout Owning geometry captured for this preparation pass; survives sibling invalidation.
 * @return New optional provider request, a snapshot identity error, or the original preparation error.
 * @note Hidden ancestors retain the existing cancellation behavior. This overload neither pins
 * nor starts provider work; the explicit CellSize overload remains available.
 */
template <class T, class Geometry>
  requires std::same_as<Geometry, LayoutSnapshot>
std::expected<std::optional<ModelRequest<T>>, TreeError>
prepareVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node, const ModelFrame<T> &frame, const Geometry &layout) {
  auto viewport = layout.viewport(node);

  if (!viewport) {
    return std::unexpected(viewport.error());
  }

  return prepareVirtualTable(tree, node, frame, *viewport);
}

/** @brief Prepare a chart using its captured local layout extent.
 * @tparam T Immutable row type accepted by the chart's OHLC accessor.
 * @tparam Geometry Deduced LayoutSnapshot; deduction preserves existing braced CellSize calls.
 * @param tree Borrowed UI owner; call outside its frame.
 * @param node Checked observing chart handle from this tree.
 * @param frame Shared model pin retained across all sibling preparations and layout retries.
 * @param layout Owning geometry captured for this preparation pass; hidden ancestors yield zero size.
 * @param first Zero-based first logical bucket, clamped by the existing chart preparation.
 * @return New optional provider request, a snapshot identity error, or the original preparation error.
 * @note Zero size cancels prior chart work. Caller owns provider dispatch; no new runtime is created.
 */
template <class T, class Geometry>
  requires std::same_as<Geometry, LayoutSnapshot>
std::expected<std::optional<ModelRequest<T>>, TreeError> prepareChart(WidgetTree &tree, Node<Chart<T>> node,
                                                                      const ModelFrame<T> &frame,
                                                                      const Geometry &layout, std::uint64_t first = 0) {
  auto viewport = layout.viewport(node);

  if (!viewport) {
    return std::unexpected(viewport.error());
  }

  return prepareChart(tree, node, frame, *viewport, first);
}
} // namespace portals
