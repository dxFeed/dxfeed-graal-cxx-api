// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <algorithm>
#include <array>
#include <cmath>
#include <portals/canvas.hpp>

namespace portals {
Canvas::Canvas(PaintContext &context, CellRect clip, const GraphemeText *brush) noexcept
    : context_(context), clip_(clip), brush_(brush) {
}

std::expected<void, TreeError> Canvas::point(CellPoint position, CellStyle style) {
  if (brush_ && (brush_->graphemes().size() != 1 || brush_->graphemes()[0].cells != 1)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  if (!clip_.contains(position)) {
    return {};
  }

  if (brush_) {
    const std::array<RenderCommand, 1> commands{TextCommand{position, *brush_, clip_, style}};

    return context_.submit(commands);
  }

  const auto area = CellRect::from(position, {1, 1});

  if (!area) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  const std::array<RenderCommand, 1> commands{FillCommand{*area, clip_, style}};

  return context_.submit(commands);
}

std::expected<void, TreeError> Canvas::line(CellPoint first, CellPoint last, CellStyle style) {
  const auto dx = static_cast<std::int64_t>(last.x) - first.x;
  const auto dy = static_cast<std::int64_t>(last.y) - first.y;

  const auto steps = std::max(std::abs(dx), std::abs(dy));

  if (steps == 0) {
    return point(first, style);
  }

  if (clip_.size().width == 0 || clip_.size().height == 0) {
    return {};
  }

  // Restrict iteration before rasterizing: clipping an already rasterized billion-cell
  // segment would violate viewport-bounded work. The major coordinate advances exactly one.
  const bool horizontal = std::abs(dx) >= std::abs(dy);

  const auto start = horizontal ? first.x : first.y;
  const auto direction = (horizontal ? dx : dy) > 0 ? 1 : -1;

  const auto low = horizontal ? clip_.origin().x : clip_.origin().y;
  const auto high = (horizontal ? clip_.right() : clip_.bottom()) - 1;

  const auto lower = (static_cast<std::int64_t>(low) - start) * direction;
  const auto upper = (static_cast<std::int64_t>(high) - start) * direction;

  const auto begin = std::max<std::int64_t>(0, std::min(lower, upper));
  const auto end = std::min(steps, std::max(lower, upper));

  for (auto i = begin; i <= end; ++i) {
    const auto ratio = static_cast<double>(i) / static_cast<double>(steps);
    const CellPoint position{static_cast<std::int32_t>(std::llround(first.x + dx * ratio)),
                             static_cast<std::int32_t>(std::llround(first.y + dy * ratio))};

    auto result = point(position, style);

    if (!result) {
      return result;
    }
  }

  return {};
}
} // namespace portals
