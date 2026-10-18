// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <expected>

namespace portals {

/** @brief Failure of a checked cell-coordinate operation. */
enum class GeometryError {
  /** @brief An origin or exclusive end would be outside the int32_t range. */
  COORDINATE_OVERFLOW
};

/**
 * @brief An owning value in terminal columns/rows, x right and y down.
 *
 * Negative coordinates are valid. No implicit Canvas/GUI unit conversion exists.
 * Values have independent lifetime, no thread affinity and no invalidation.
 * Concurrent read-only use is safe; shared mutation requires caller synchronization.
 */
struct CellPoint {
  /** @brief Horizontal coordinate in terminal columns. */
  std::int32_t x{};
  /** @brief Vertical coordinate in terminal rows. */
  std::int32_t y{};

  /** @brief Compare both coordinates; no errors or invalidation.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellPoint &other) const noexcept = default;
};

/**
 * @brief An owning available extent in terminal columns/rows.
 *
 * Signed inputs may be negative; normalized() clamps each negative component to
 * zero. No implicit Canvas/GUI conversion exists. Values have independent lifetime,
 * no thread affinity and no invalidation. Shared mutation requires synchronization.
 */
struct CellSize {
  /** @brief Available number of columns, possibly negative before normalization. */
  std::int32_t width{};
  /** @brief Available number of rows, possibly negative before normalization. */
  std::int32_t height{};

  /** @brief Return an owning nonnegative extent; no errors or invalidation.
   * @return A copy with each negative extent replaced by zero.
   */
  [[nodiscard]] CellSize normalized() const noexcept;

  /** @brief Compare stored components; no normalization, errors or invalidation.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellSize &other) const noexcept = default;
};

/**
 * @brief Translate a cell point by an explicit columns/rows offset.
 * @param point Value to translate, copied without borrowing storage.
 * @param offset Signed cell displacement, copied without borrowing storage.
 * @return An independent point, or GeometryError::COORDINATE_OVERFLOW.
 * @note Any thread; no allocation, exceptions, mutation or invalidation.
 */
[[nodiscard]] std::expected<CellPoint, GeometryError> translated(CellPoint point, CellPoint offset) noexcept;

/**
 * @brief An owning, invariant-preserving half-open cell rectangle.
 *
 * The covered cells are [x, right()) x [y, bottom()). Extents are nonnegative;
 * origin and exclusive ends fit int32_t. A zero extent on either axis is empty.
 * All results own their data. No references are borrowed, no allocation occurs,
 * and no implicit Canvas/GUI conversion exists. Values are thread-agnostic;
 * concurrent reads are safe and shared assignment requires synchronization.
 * Operations do not invalidate any values or trigger layout/paint invalidation.
 */
class CellRect {
public:
  /** @brief Create the empty rectangle at (0, 0); no errors. */
  CellRect() noexcept = default;

  /**
   * @brief Create a rectangle after normalizing negative available extents to zero.
   * @param origin Signed cell origin, copied without borrowing storage.
   * @param available Signed available cell extents, copied and normalized.
   * @return An independent rectangle, or COORDINATE_OVERFLOW if an end cannot fit.
   * @note No exceptions or invalidation; an empty axis still has a checked origin.
   */
  [[nodiscard]] static std::expected<CellRect, GeometryError> from(CellPoint origin, CellSize available) noexcept;

  /** @brief Return the cell origin by value; no errors.
   * @return The inclusive top-left cell coordinate, copied by value.
   */
  [[nodiscard]] CellPoint origin() const noexcept;
  /** @brief Return the normalized cell extents by value; no errors.
   * @return Nonnegative width in columns and height in rows.
   */
  [[nodiscard]] CellSize size() const noexcept;
  /** @brief Return the representable exclusive horizontal end; no errors.
   * @return The exclusive x coordinate, including for empty rectangles.
   */
  [[nodiscard]] std::int32_t right() const noexcept;
  /** @brief Return the representable exclusive vertical end; no errors.
   * @return The exclusive y coordinate, including for empty rectangles.
   */
  [[nodiscard]] std::int32_t bottom() const noexcept;
  /** @brief Whether either extent is zero; no errors.
   * @return True when width or height is zero.
   */
  [[nodiscard]] bool empty() const noexcept;

  /** @brief Test half-open point membership; an empty rectangle contains no point.
   * @param point Cell coordinate tested against the half-open bounds.
   * @return True only when both coordinates lie inside a nonempty rectangle.
   */
  [[nodiscard]] bool contains(CellPoint point) const noexcept;
  /** @brief Test cell-set inclusion; every empty rectangle is a subset of every rectangle.
   * @param rect Rectangle whose covered cell set is tested.
   * @return True for an empty rect or when all its covered cells lie inside this rectangle.
   */
  [[nodiscard]] bool contains(CellRect rect) const noexcept;

  /**
   * @brief Intersect with a clipping rectangle; no errors or invalidation.
   * @param clip Owning-value clipping bounds, copied without borrowing storage.
   * @return An independent rectangle at the per-axis maximum of the input origins.
   *
   * Each disjoint/touching axis has zero extent, independently of the other axis.
   * Empty results retain that deterministic origin and the other axis's overlap;
   * their position does not imply point membership. No unchecked subtraction occurs.
   */
  [[nodiscard]] CellRect intersection(CellRect clip) const noexcept;

  /**
   * @brief Return an owning translated rectangle, preserving its extents.
   * @param offset Signed cell displacement, copied without borrowing storage.
   * @return An independent rectangle or COORDINATE_OVERFLOW for an origin or end.
   * @note No exceptions, mutation or invalidation, including on failure.
   */
  [[nodiscard]] std::expected<CellRect, GeometryError> translated(CellPoint offset) const noexcept;

  /** @brief Compare origin and extents, including the position of empty rectangles.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellRect &other) const noexcept = default;

private:
  CellRect(CellPoint origin, CellSize size) noexcept;

  CellPoint origin_{};
  CellSize size_{};
};

} // namespace portals
