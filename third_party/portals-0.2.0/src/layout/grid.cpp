// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "grid_detail.hpp"
#include <new>

namespace portals {
namespace {
using grid_detail::Wide;

bool within(std::size_t first, std::size_t count, std::size_t bound) {
  return count > 0 && first < bound && count <= bound - first;
}

std::expected<void, TreeError> validate(const GridOptions &options) {
  if (options.columns.empty() || options.rows.empty() || options.max_tracks_per_axis == 0 ||
      options.max_placements == 0 || options.column_gap < 0 || options.row_gap < 0 ||
      !grid_detail::valid(options.padding)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  if (options.columns.size() > options.max_tracks_per_axis || options.rows.size() > options.max_tracks_per_axis ||
      options.cells.size() > options.max_placements) {
    return std::unexpected(TreeError::NODE_LIMIT);
  }

  if (options.columns.size() > static_cast<std::size_t>(grid_detail::CELL_LIMIT) ||
      options.rows.size() > static_cast<std::size_t>(grid_detail::CELL_LIMIT)) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  for (const auto &axis : {std::span(options.columns), std::span(options.rows)}) {
    for (auto track : axis) {
      if (!grid_detail::valid(track)) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }
    }
  }

  for (std::size_t i = 0; i < options.cells.size(); ++i) {
    const auto &cell = options.cells[i];

    if (cell.node.session == 0 || cell.node.generation == 0 || !within(cell.row, cell.row_span, options.rows.size()) ||
        !within(cell.column, cell.column_span, options.columns.size()) || !grid_detail::valid(cell.horizontal) ||
        !grid_detail::valid(cell.vertical)) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    for (std::size_t j = 0; j < i; ++j) {
      if (cell.node == options.cells[j].node) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }
    }
  }

  return {};
}

const GridCell *placement(const GridOptions &options, NodeId node) {
  const auto found = std::ranges::find_if(options.cells, [&](const GridCell &cell) {
    return cell.node == node;
  });

  return found == options.cells.end() ? nullptr : &*found;
}

// Only intrinsic, nonfixed tracks absorb demand; explicit preferences remain authoritative.
void absorb(std::span<MeasuredChild> tracks, Wide demand, std::int32_t gap) {
  Wide total = Wide{gap} * static_cast<Wide>(tracks.size() - 1);

  for (const auto &track : tracks) {
    total += track.desired.width;
  }

  Wide deficit = std::max<Wide>(0, demand - total);

  while (deficit > 0) {
    Wide eligible{};

    for (const auto &track : tracks) {
      if (track.policy.width.mode != SizeMode::FIXED && track.policy.width.preferred == INTRINSIC_EXTENT &&
          track.desired.width < track.policy.width.maximum) {
        ++eligible;
      }
    }

    if (eligible == 0) {
      break;
    }

    const Wide quotient = deficit / eligible;
    bool saturated{};

    // Saturate capped tracks before assigning remainder cells to the remaining tracks.
    for (auto &track : tracks) {
      if (track.policy.width.mode == SizeMode::FIXED || track.policy.width.preferred != INTRINSIC_EXTENT ||
          track.desired.width == track.policy.width.maximum) {
        continue;
      }

      const Wide headroom = track.policy.width.maximum - track.desired.width;

      if (headroom <= quotient) {
        track.desired.width = track.policy.width.maximum;
        deficit -= headroom;
        saturated = true;
      }
    }

    // A capped track must not consume a remainder cell it could not accept. Restart
    // the equal-share calculation over remaining headroom before distributing extras.
    if (saturated) {
      continue;
    }

    Wide remainder = deficit % eligible;

    for (auto &track : tracks) {
      if (track.policy.width.mode == SizeMode::FIXED || track.policy.width.preferred != INTRINSIC_EXTENT ||
          track.desired.width == track.policy.width.maximum) {
        continue;
      }

      const Wide growth = quotient + (remainder > 0 ? 1 : 0);

      remainder = std::max<Wide>(0, remainder - 1);
      track.desired.width += static_cast<std::int32_t>(growth);
      deficit -= growth;
    }
  }
}

std::expected<std::vector<MeasuredChild>, TreeError> tracks(const GridOptions &options,
                                                            std::span<const MeasuredChild> children, bool horizontal) {
  const auto &policies = horizontal ? options.columns : options.rows;
  const auto gap = horizontal ? options.column_gap : options.row_gap;
  std::vector<MeasuredChild> result(0);

  if (policies.size() > result.max_size()) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  result.reserve(policies.size());

  for (auto policy : policies) {
    MeasuredChild track;

    track.policy.width = policy;
    track.desired.width = static_cast<std::int32_t>(grid_detail::requested(policy, 0));
    result.push_back(track);
  }

  // Single-track demand is established before spanning demand, independently of child order.
  for (bool spanning : {false, true}) {
    for (const auto &child : children) {
      const auto *cell = placement(options, child.node);

      if (!cell || !grid_detail::valid(child)) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      const auto first = horizontal ? cell->column : cell->row;
      const auto count = horizontal ? cell->column_span : cell->row_span;

      if ((count > 1) != spanning) {
        continue;
      }

      const auto policy = horizontal ? child.policy.width : child.policy.height;
      const auto desired = horizontal ? child.desired.width : child.desired.height;

      absorb(std::span(result).subspan(first, count), grid_detail::requested(policy, desired), gap);
    }
  }

  return std::expected<std::vector<MeasuredChild>, TreeError>(std::in_place, result);
}

// Reuse one-dimensional allocation for each axis. Rows use the same x/width solver
// as columns; cellRect translates its results back to two-dimensional geometry.
Row axisLayout(std::int32_t gap) {
  LinearOptions options;

  options.gap = gap;

  return Row(options);
}

std::expected<CellRect, TreeError> cellRect(const GridCell &cell, std::span<const CellRect> columns,
                                            std::span<const CellRect> rows, LayoutPadding padding) {
  const auto &left = columns[cell.column];
  const auto &right = columns[cell.column + cell.column_span - 1];
  const auto &top = rows[cell.row];
  const auto &bottom = rows[cell.row + cell.row_span - 1];
  auto dimensions = grid_detail::size(Wide{right.origin().x} + right.size().width - left.origin().x,
                                      Wide{bottom.origin().x} + bottom.size().width - top.origin().x);
  auto origin = grid_detail::size(Wide{left.origin().x} + padding.left, Wide{top.origin().x} + padding.top);

  if (!dimensions || !origin) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  auto result = CellRect::from({origin->width, origin->height}, *dimensions);

  if (!result) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  return *result;
}
} // namespace

Grid::Grid(const GridOptions &options, SizePolicy policy) : SizedWidget(policy), options_(options) {
}

GridOptions Grid::options() const {
  return options_;
}

std::expected<CellSize, TreeError> Grid::childAvailable(CellSize available) const {
  auto checked = validate(options_);

  if (!checked) {
    return std::unexpected(checked.error());
  }

  return grid_detail::inset(available, options_.padding);
}

std::expected<CellSize, TreeError> Grid::measureChildren(std::span<const MeasuredChild> children) const {
  auto checked = validate(options_);

  if (!checked) {
    return std::unexpected(checked.error());
  }

  if (children.size() > options_.max_placements) {
    return std::unexpected(TreeError::NODE_LIMIT);
  }

  try {
    auto columns = tracks(options_, children, true);
    auto rows = tracks(options_, children, false);

    if (!columns || !rows) {
      return std::unexpected(columns ? rows.error() : columns.error());
    }

    auto width = axisLayout(options_.column_gap).measureChildren(*columns);
    auto height = axisLayout(options_.row_gap).measureChildren(*rows);

    if (!width || !height) {
      return std::unexpected(width ? height.error() : width.error());
    }

    return grid_detail::padded({width->width, height->width}, options_.padding);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<std::vector<CellRect>, TreeError> Grid::arrangeChildren(std::span<const MeasuredChild> children,
                                                                      CellSize available) const {
  auto content = childAvailable(available);

  if (!content) {
    return std::unexpected(content.error());
  }

  if (children.size() > options_.max_placements) {
    return std::unexpected(TreeError::NODE_LIMIT);
  }

  try {
    auto columns = tracks(options_, children, true);
    auto rows = tracks(options_, children, false);

    if (!columns || !rows) {
      return std::unexpected(columns ? rows.error() : columns.error());
    }

    auto horizontal = axisLayout(options_.column_gap).arrangeChildren(*columns, {content->width, 0});
    auto vertical = axisLayout(options_.row_gap).arrangeChildren(*rows, {content->height, 0});

    if (!horizontal || !vertical) {
      return std::unexpected(horizontal ? vertical.error() : horizontal.error());
    }

    std::vector<CellRect> result(0);

    if (children.size() > result.max_size()) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    result.reserve(children.size());

    for (const auto &child : children) {
      const auto &cell = *placement(options_, child.node);
      auto slot = cellRect(cell, *horizontal, *vertical, options_.padding);

      if (!slot) {
        return std::unexpected(slot.error());
      }

      auto placed = grid_detail::place(child, *slot, cell.horizontal, cell.vertical);

      if (!placed) {
        return std::unexpected(placed.error());
      }

      result.push_back(*placed);
    }

    return std::expected<std::vector<CellRect>, TreeError>(std::in_place, result);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<void, TreeError> setGridOptions(WidgetTree &tree, Node<Grid> node, const GridOptions &options) {
  auto checked = validate(options);

  if (!checked) {
    return checked;
  }

  try {
    GridOptions replacement(options);
    auto result = tree.edit(node, [&](Grid &widget) {
      widget.options_.columns.swap(replacement.columns);
      widget.options_.rows.swap(replacement.rows);
      widget.options_.cells.swap(replacement.cells);
      widget.options_.column_gap = replacement.column_gap;
      widget.options_.row_gap = replacement.row_gap;
      widget.options_.padding = replacement.padding;
      widget.options_.max_tracks_per_axis = replacement.max_tracks_per_axis;
      widget.options_.max_placements = replacement.max_placements;
    });

    if (!result) {
      return result;
    }

    return tree.invalidate(node, Invalidation::LAYOUT);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}
} // namespace portals
