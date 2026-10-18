// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <algorithm>
#include <portals/grid.hpp>

namespace portals::grid_detail {
using Wide = std::int64_t;
inline constexpr Wide CELL_LIMIT = std::numeric_limits<std::int32_t>::max();

inline bool valid(LayoutAlign align) {
  return align == LayoutAlign::START || align == LayoutAlign::CENTER || align == LayoutAlign::END ||
         align == LayoutAlign::STRETCH;
}

inline bool valid(LayoutPadding padding) {
  return padding.left >= 0 && padding.top >= 0 && padding.right >= 0 && padding.bottom >= 0;
}

inline bool valid(AxisSize policy) {
  return policy.minimum >= 0 && policy.maximum >= policy.minimum && policy.preferred >= INTRINSIC_EXTENT &&
         policy.weight > 0 &&
         (policy.mode == SizeMode::AUTOMATIC || policy.mode == SizeMode::FIXED || policy.mode == SizeMode::FLEX);
}

inline bool valid(const MeasuredChild &child) {
  return valid(child.policy.width) && valid(child.policy.height) && child.desired.width >= 0 &&
         child.desired.height >= 0;
}

inline Wide requested(AxisSize policy, std::int32_t intrinsic) {
  return std::clamp<Wide>(policy.preferred == INTRINSIC_EXTENT ? intrinsic : policy.preferred, policy.minimum,
                          policy.maximum);
}

inline std::expected<CellSize, TreeError> size(Wide width, Wide height) {
  if (width < 0 || height < 0 || width > CELL_LIMIT || height > CELL_LIMIT) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  return CellSize{static_cast<std::int32_t>(width), static_cast<std::int32_t>(height)};
}

inline std::expected<CellSize, TreeError> inset(CellSize available, LayoutPadding padding) {
  if (!valid(padding)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  available = available.normalized();

  return size(std::max<Wide>(0, Wide{available.width} - padding.left - padding.right),
              std::max<Wide>(0, Wide{available.height} - padding.top - padding.bottom));
}

inline std::expected<CellSize, TreeError> padded(CellSize content, LayoutPadding padding) {
  return size(Wide{content.width} + padding.left + padding.right, Wide{content.height} + padding.top + padding.bottom);
}

inline Wide extent(AxisSize policy, std::int32_t intrinsic, std::int32_t available, LayoutAlign align) {
  const auto wanted = requested(policy, intrinsic);

  if (policy.mode == SizeMode::FIXED) {
    return wanted;
  }

  return std::clamp<Wide>(
    (align == LayoutAlign::STRETCH || policy.mode == SizeMode::FLEX) ? available : std::min<Wide>(wanted, available),
    policy.minimum, policy.maximum);
}

inline Wide offset(Wide space, Wide child, LayoutAlign align) {
  const auto spare = std::max<Wide>(0, space - child);

  if (align == LayoutAlign::CENTER) {
    return spare / 2;
  }

  return align == LayoutAlign::END ? spare : 0;
}

inline std::expected<CellRect, TreeError> place(const MeasuredChild &child, CellRect slot, LayoutAlign horizontal,
                                                LayoutAlign vertical) {
  if (!valid(child) || !valid(horizontal) || !valid(vertical)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  const auto width = extent(child.policy.width, child.desired.width, slot.size().width, horizontal);
  const auto height = extent(child.policy.height, child.desired.height, slot.size().height, vertical);
  const auto origin = size(Wide{slot.origin().x} + offset(slot.size().width, width, horizontal),
                           Wide{slot.origin().y} + offset(slot.size().height, height, vertical));
  const auto dimensions = size(width, height);

  if (!origin || !dimensions) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  auto result = CellRect::from({origin->width, origin->height}, *dimensions);

  if (!result) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  return *result;
}
} // namespace portals::grid_detail
