// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/virtual_list.hpp>

#include <array>

namespace portals::detail {
std::expected<ListWindow, TreeError> listWindow(std::uint64_t count, std::uint64_t first, CellSize viewport,
                                                VirtualListOptions options) {
  if (options.row_height <= 0 || viewport.width < 0 || viewport.height < 0) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  first = count == 0 ? 0 : std::min(first, count - 1);

  if (viewport.width == 0 || viewport.height == 0 || count == 0) {
    return ListWindow{{first, 0}, {first, 0}};
  }

  // Count only local viewport rows. Subtractions bound each term before any addition, even at UINT64_MAX.
  const auto height = static_cast<std::uint64_t>(viewport.height);
  const auto row_height = static_cast<std::uint64_t>(options.row_height);
  const auto visible = std::min(count - first, height / row_height + (height % row_height != 0));
  const auto before = std::min<std::uint64_t>(first, options.overscan_rows);
  const auto after = std::min<std::uint64_t>(count - first - visible, options.overscan_rows);
  const auto total = before + visible;

  if (total > std::numeric_limits<std::size_t>::max() || after > std::numeric_limits<std::size_t>::max() - total) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return ListWindow{{first, static_cast<std::size_t>(visible)},
                    {first - before, static_cast<std::size_t>(total + after)}};
}

std::expected<void, TreeError> paintList(PaintContext &context, CellSize viewport, VirtualListOptions options,
                                         std::span<const ListRow> rows, std::optional<RowId> selected) {
  auto bounds = CellRect::from({}, viewport);

  if (!bounds) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  // Text starts on the first cell row. Clip the final partial row before surface/ancestor clipping by the sink.
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto y = static_cast<std::int64_t>(i) * options.row_height;
    // Count only local viewport rows. Subtractions bound each term before any addition, even at UINT64_MAX.
    const auto height = std::min<std::int64_t>(options.row_height, viewport.height - y);
    auto area = CellRect::from({0, static_cast<std::int32_t>(y)}, {viewport.width, static_cast<std::int32_t>(height)});

    if (!area) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    const auto style = selected && rows[i].id == selected ? options.selected : options.normal;
    const std::array<RenderCommand, 2> commands{FillCommand{*area, *bounds, style},
                                                TextCommand{area->origin(), rows[i].text, *area, style}};
    auto result = context.submit(commands);

    if (!result) {
      return result;
    }
  }

  return {};
}

std::expected<bool, TreeError> listShown(WidgetTree &tree, Node<Widget> node) {
  while (node.id().generation != 0) {
    auto state = tree.inspect(node);

    if (!state) {
      return std::unexpected(state.error());
    }

    if (!state->visible) {
      return false;
    }

    node = state->parent;
  }

  return true;
}
} // namespace portals::detail
