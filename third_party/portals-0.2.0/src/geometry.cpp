// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/geometry.hpp>

#include <algorithm>
#include <limits>

namespace portals {
namespace {
bool fitsCoordinate(std::int64_t value) noexcept {
  return value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max();
}

std::int32_t overlapExtent(std::int32_t begin, std::int32_t end) noexcept {
  if (end <= begin) {
    return 0;
  }

  // Intersection is no wider than either valid input extent.
  return static_cast<std::int32_t>(std::int64_t{end} - begin);
}
} // namespace

CellSize CellSize::normalized() const noexcept {
  return {std::max(width, std::int32_t{0}), std::max(height, std::int32_t{0})};
}

std::expected<CellPoint, GeometryError> translated(CellPoint point, CellPoint offset) noexcept {
  const auto x = std::int64_t{point.x} + offset.x;
  const auto y = std::int64_t{point.y} + offset.y;

  if (!fitsCoordinate(x) || !fitsCoordinate(y)) {
    return std::unexpected{GeometryError::COORDINATE_OVERFLOW};
  }

  return CellPoint{static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
}

CellRect::CellRect(CellPoint origin, CellSize size) noexcept : origin_(origin), size_(size) {
}

std::expected<CellRect, GeometryError> CellRect::from(CellPoint origin, CellSize available) noexcept {
  const auto size = available.normalized();
  const auto right = std::int64_t{origin.x} + size.width;
  const auto bottom = std::int64_t{origin.y} + size.height;

  if (!fitsCoordinate(right) || !fitsCoordinate(bottom)) {
    return std::unexpected{GeometryError::COORDINATE_OVERFLOW};
  }

  return CellRect{origin, size};
}

CellPoint CellRect::origin() const noexcept {
  return origin_;
}

CellSize CellRect::size() const noexcept {
  return size_;
}

std::int32_t CellRect::right() const noexcept {
  return static_cast<std::int32_t>(std::int64_t{origin_.x} + size_.width);
}

std::int32_t CellRect::bottom() const noexcept {
  return static_cast<std::int32_t>(std::int64_t{origin_.y} + size_.height);
}

bool CellRect::empty() const noexcept {
  return size_.width == 0 || size_.height == 0;
}

bool CellRect::contains(CellPoint point) const noexcept {
  return point.x >= origin_.x && point.y >= origin_.y && point.x < right() && point.y < bottom();
}

bool CellRect::contains(CellRect rect) const noexcept {
  if (rect.empty()) {
    return true;
  }

  return !empty() && rect.origin_.x >= origin_.x && rect.origin_.y >= origin_.y && rect.right() <= right() &&
         rect.bottom() <= bottom();
}

CellRect CellRect::intersection(CellRect clip) const noexcept {
  const CellPoint origin{std::max(origin_.x, clip.origin_.x), std::max(origin_.y, clip.origin_.y)};
  const CellSize size{overlapExtent(origin.x, std::min(right(), clip.right())),
                      overlapExtent(origin.y, std::min(bottom(), clip.bottom()))};

  return CellRect{origin, size};
}

std::expected<CellRect, GeometryError> CellRect::translated(CellPoint offset) const noexcept {
  const auto origin = portals::translated(origin_, offset);

  if (!origin) {
    return std::unexpected{origin.error()};
  }

  return from(*origin, size_);
}

} // namespace portals
