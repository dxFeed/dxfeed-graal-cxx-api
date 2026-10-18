// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/geometry.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

namespace {
using portals::CellPoint;
using portals::CellRect;
using portals::CellSize;
using portals::GeometryError;

int failures = 0;

void check(bool condition, const char *message) {
  if (!condition) {
    if (failures < 10) {
      std::cerr << message << '\n';
    }

    ++failures;
  }
}

bool oracleContains(CellRect rect, CellPoint point) {
  const auto origin = rect.origin();
  const auto size = rect.size();
  const auto right = std::int64_t{origin.x} + size.width;
  const auto bottom = std::int64_t{origin.y} + size.height;

  return point.x >= origin.x && point.y >= origin.y && point.x < right && point.y < bottom;
}

void boundaries() {
  constexpr auto LOW = std::numeric_limits<std::int32_t>::min();
  constexpr auto HIGH = std::numeric_limits<std::int32_t>::max();

  check(CellSize{-5, 7}.normalized() == CellSize{0, 7}, "negative width normalization");
  check(CellSize{7, -5}.normalized() == CellSize{7, 0}, "negative height normalization");
  check(CellSize{LOW, LOW}.normalized() == CellSize{}, "minimum extents normalization");

  const auto negative = CellRect::from({-4, -3}, {-9, 2});

  check(negative && negative->origin() == CellPoint{-4, -3} && negative->size() == CellSize{0, 2},
        "negative available width is empty at the supplied origin");
  check(negative && negative->empty() && !negative->contains(CellPoint{-4, -3}), "empty contains no point");

  const auto full = CellRect::from({LOW, LOW}, {HIGH, HIGH});

  check(full && full->right() == -1 && full->bottom() == -1, "widest representable size at minimum origin");
  check(full && full->contains(CellPoint{LOW, LOW}) && !full->contains(CellPoint{-1, -1}), "half-open limits");

  const auto high_empty = CellRect::from({HIGH, HIGH}, {0, LOW});

  check(high_empty && high_empty->empty() && high_empty->right() == HIGH && high_empty->bottom() == HIGH,
        "empty maximum origin is valid");

  const auto overflow_x = CellRect::from({HIGH, 0}, {1, 1});
  const auto overflow_y = CellRect::from({0, HIGH}, {1, 1});

  check(!overflow_x && overflow_x.error() == GeometryError::COORDINATE_OVERFLOW, "right overflow is explicit");
  check(!overflow_y && overflow_y.error() == GeometryError::COORDINATE_OVERFLOW, "bottom overflow is explicit");

  const auto corner = CellRect::from({HIGH - 1, HIGH - 1}, {1, 1});

  check(corner && corner->contains(CellPoint{HIGH - 1, HIGH - 1}) && !corner->contains(CellPoint{HIGH, HIGH}),
        "last representable cell and exclusive end");
  check(corner && !corner->translated({1, 0}), "translation checks end as well as origin");
  check(full && !full->translated({-1, 0}), "translation checks minimum origin");
  check(high_empty && !high_empty->translated({0, 1}), "empty translation still checks origin");
  check(full && full->translated({HIGH, HIGH}) && full->translated({HIGH, HIGH})->origin() == CellPoint{-1, -1},
        "wide safe translation");

  check(!portals::translated(CellPoint{HIGH, 0}, {1, 0}), "point positive overflow");
  check(!portals::translated(CellPoint{0, LOW}, {0, -1}), "point negative overflow");
  check(portals::translated(CellPoint{LOW, HIGH}, {HIGH, LOW}) == CellPoint{-1, -1}, "point wide addition");

  if (full && corner && high_empty) {
    const auto disjoint = full->intersection(*corner);

    check(disjoint.origin() == corner->origin() && disjoint.size() == CellSize{} && disjoint.empty(),
          "far disjoint intersection has a representable deterministic empty origin");
    check(full->contains(*high_empty) && corner->contains(*high_empty), "empty rectangle is a subset everywhere");
    check(!high_empty->contains(*corner), "empty rectangle cannot contain a nonempty rectangle");
  }

  const auto left = CellRect::from({0, 0}, {2, 3});
  const auto right = CellRect::from({2, 0}, {2, 3});

  if (left && right) {
    check(left->intersection(*right).size() == CellSize{0, 3}, "touching edges have no shared interior");
    check(!left->contains(CellPoint{2, 1}), "right edge is excluded");
  }
}

void boundedProperties() {
  constexpr std::array<std::int32_t, 3> ORIGINS{-2, 0, 2};
  constexpr std::array<std::int32_t, 4> EXTENTS{-2, 0, 1, 4};
  constexpr CellPoint OFFSET{3, -2};
  std::vector<CellRect> rectangles;

  for (const auto x : ORIGINS) {
    for (const auto y : ORIGINS) {
      for (const auto width : EXTENTS) {
        for (const auto height : EXTENTS) {
          const auto rect = CellRect::from({x, y}, {width, height});

          check(rect.has_value(), "bounded rectangle creation");

          if (rect) {
            rectangles.push_back(*rect);
          }
        }
      }
    }
  }

  for (const auto a : rectangles) {
    check(a.intersection(a) == a, "intersection idempotence including degenerate axes");
    check(a.contains(a), "containment reflexivity");

    const auto moved_a = a.translated(OFFSET);

    check(moved_a && moved_a->translated({-OFFSET.x, -OFFSET.y}) == a, "translation inverse");

    for (const auto b : rectangles) {
      const auto intersection = a.intersection(b);

      check(intersection == b.intersection(a), "intersection commutativity");
      check(a.contains(intersection) && b.contains(intersection), "intersection subset identity");

      const auto moved_b = b.translated(OFFSET);

      check(moved_a && moved_b && intersection.translated(OFFSET) == moved_a->intersection(*moved_b),
            "intersection commutes with translation");
      check(moved_a && moved_b && a.contains(b) == moved_a->contains(*moved_b), "containment translation identity");

      bool b_is_subset = true;

      for (std::int32_t x = -3; x <= 6; ++x) {
        for (std::int32_t y = -3; y <= 6; ++y) {
          const CellPoint point{x, y};
          const bool in_a = oracleContains(a, point);
          const bool in_b = oracleContains(b, point);

          check(a.contains(point) == in_a, "point containment agrees with independent interval oracle");
          check(intersection.contains(point) == (in_a && in_b), "intersection agrees with cell-set oracle");
          check(moved_a && moved_a->contains({x + OFFSET.x, y + OFFSET.y}) == in_a, "point translation identity");
          b_is_subset = b_is_subset && (!in_b || in_a);
        }
      }

      check(a.contains(b) == b_is_subset, "rectangle containment agrees with bounded cell-set oracle");
    }
  }
}

void extremeProperties() {
  constexpr auto LOW = std::numeric_limits<std::int32_t>::min();
  constexpr auto HIGH = std::numeric_limits<std::int32_t>::max();
  constexpr std::array<std::int32_t, 5> VALUES{LOW, -1, 0, 1, HIGH};
  std::vector<CellRect> rectangles;

  for (const auto x : VALUES) {
    for (const auto y : VALUES) {
      for (const auto width : VALUES) {
        for (const auto height : VALUES) {
          const auto expected_width = width < 0 ? 0 : width;
          const auto expected_height = height < 0 ? 0 : height;
          const auto right = std::int64_t{x} + expected_width;
          const auto bottom = std::int64_t{y} + expected_height;
          const auto rect = CellRect::from({x, y}, {width, height});
          const bool representable = right <= HIGH && bottom <= HIGH;

          check(rect.has_value() == representable, "extreme factory agrees with independent wide arithmetic");

          if (!rect) {
            check(rect.error() == GeometryError::COORDINATE_OVERFLOW, "extreme factory reports overflow");

            continue;
          }

          rectangles.push_back(*rect);
          check(rect->origin() == CellPoint{x, y} && rect->size() == CellSize{expected_width, expected_height},
                "extreme factory preserves origin and normalizes independently");

          for (const auto dx : VALUES) {
            for (const auto dy : VALUES) {
              const auto moved_x = std::int64_t{x} + dx;
              const auto moved_y = std::int64_t{y} + dy;
              const bool can_move = moved_x >= LOW && moved_x <= HIGH && moved_y >= LOW && moved_y <= HIGH &&
                                    right + dx >= LOW && right + dx <= HIGH && bottom + dy >= LOW &&
                                    bottom + dy <= HIGH;
              const auto moved = rect->translated({dx, dy});

              check(moved.has_value() == can_move, "extreme translation checks origins and ends");

              if (moved) {
                check(moved->origin() ==
                          CellPoint{static_cast<std::int32_t>(moved_x), static_cast<std::int32_t>(moved_y)} &&
                        moved->size() == rect->size(),
                      "successful extreme translation preserves extents");
              } else {
                check(moved.error() == GeometryError::COORDINATE_OVERFLOW, "extreme translation reports overflow");
              }
            }
          }
        }
      }
    }
  }

  for (const auto a : rectangles) {
    check(a.intersection(a) == a, "extreme intersection idempotence");

    for (const auto b : rectangles) {
      const auto intersection = a.intersection(b);

      check(intersection == b.intersection(a), "extreme intersection commutativity");
      check(a.contains(intersection) && b.contains(intersection), "extreme intersection subset identity");

      for (const auto x : VALUES) {
        for (const auto y : VALUES) {
          const CellPoint point{x, y};

          check(intersection.contains(point) == (oracleContains(a, point) && oracleContains(b, point)),
                "extreme intersection agrees with independent interval oracle");
        }
      }
    }
  }
}

} // namespace

static_assert(!std::is_convertible_v<CellPoint, CellSize>);
static_assert(!std::is_convertible_v<CellSize, CellPoint>);
static_assert(!std::is_convertible_v<int, CellRect>);
static_assert(std::is_trivially_copyable_v<CellRect>);

int main() {
  boundaries();
  boundedProperties();
  extremeProperties();

  return failures == 0 ? 0 : 1;
}
