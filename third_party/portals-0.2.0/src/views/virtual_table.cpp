// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/virtual_table.hpp>

#include <array>

namespace portals::detail {
std::expected<void, TreeError> paintTable(PaintContext &context, CellSize viewport, VirtualTableOptions options,
                                          std::span<const TableHeading> headings, std::span<const TableRow> rows,
                                          std::optional<RowId> selected) {
  auto bounds = CellRect::from({}, viewport);

  if (!bounds) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  if (viewport.width == 0 || viewport.height == 0) {
    return {};
  }

  const auto header_area = *CellRect::from({}, {viewport.width, TABLE_HEADER_HEIGHT});
  const std::array<RenderCommand, 2> background{FillCommand{*bounds, *bounds, options.normal},
                                                FillCommand{header_area, *bounds, options.header}};
  auto result = context.submit(background);

  if (!result) {
    return result;
  }

  for (const auto &heading : headings) {
    const auto area = *CellRect::from({heading.x, 0}, {heading.width, TABLE_HEADER_HEIGHT});
    const std::array<RenderCommand, 1> commands{TextCommand{area.origin(), heading.text, area, options.header}};

    result = context.submit(commands);

    if (!result) {
      return result;
    }
  }

  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto y = TABLE_HEADER_HEIGHT + static_cast<std::int64_t>(i) * options.row_height;
    const auto height = std::min<std::int64_t>(options.row_height, viewport.height - y);
    const auto style = selected && rows[i].id == selected ? options.selected : options.normal;
    const auto row_area =
      *CellRect::from({0, static_cast<std::int32_t>(y)}, {viewport.width, static_cast<std::int32_t>(height)});
    const std::array<RenderCommand, 1> fill{FillCommand{row_area, *bounds, style}};

    result = context.submit(fill);

    if (!result) {
      return result;
    }

    for (std::size_t column = 0; column < headings.size(); ++column) {
      const auto &heading = headings[column];
      const auto area =
        *CellRect::from({heading.x, static_cast<std::int32_t>(y)}, {heading.width, static_cast<std::int32_t>(height)});
      // A per-cell clip prevents long/wide graphemes from crossing a column boundary.
      const std::array<RenderCommand, 1> text{TextCommand{area.origin(), rows[i].cells[column], area, style}};

      result = context.submit(text);

      if (!result) {
        return result;
      }
    }
  }

  return {};
}
} // namespace portals::detail
