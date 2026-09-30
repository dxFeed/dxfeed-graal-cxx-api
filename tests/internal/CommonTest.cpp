// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/internal/Common.hpp>

#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

using namespace dxfcpp;

namespace {

double nanWithPayload(std::uint64_t payload) {
    return std::bit_cast<double>(UINT64_C(0x7ff8000000000000) | payload);
}

template <typename T, typename U>
concept ApproximatelyComparable = requires(T a, U b) { math::approximatelyEquals(a, b, 1.0e-12, 0.0); };

static_assert(ApproximatelyComparable<float, double>);
static_assert(!ApproximatelyComparable<std::int64_t, std::int64_t>);

// The bit operations are constexpr, and Max is the first type of the largest size.
static_assert(sal(std::int64_t{1}, 3) == 8);
static_assert(shr(std::int8_t{-1}, 4) == 0x0F);
static_assert(andOp(std::int64_t{-1}, std::uint8_t{0xF0}) == 0xF0);
static_assert(std::is_same_v<Max<std::int32_t, std::uint32_t>, std::int32_t>);
static_assert(std::is_same_v<Max<std::uint16_t, std::int8_t, std::int16_t>, std::uint16_t>);
static_assert(std::is_same_v<Max<std::int8_t, std::int64_t, std::uint64_t>, std::int64_t>);

} // namespace

TEST_CASE("floorDiv and floorMod should match Java Math.floorDiv and Math.floorMod, including the int64 limits") {
    constexpr auto MIN = std::numeric_limits<std::int64_t>::min();
    constexpr auto MAX = std::numeric_limits<std::int64_t>::max();

    struct Case {
        std::int64_t x;
        std::int64_t y;
        std::int64_t floorDiv;
        std::int64_t floorMod;
    };

    // The results of the Java API for the same arguments.
    const Case cases[]{
        {7, 1000, 0, 7},
        {-1, 1000, -1, 999},
        {1, -1000, -1, -999},
        {-1000, 1000, -1, 0},
        {-7, -1000, 0, -7},
        {MAX, 1000, 9'223'372'036'854'775, 807},
        {MIN, 1000, -9'223'372'036'854'776, 192},
        {MIN, 1'000'000, -9'223'372'036'855, 224'192},
        {MIN, -1, MIN, 0},
        {MAX, -1, -MAX, 0},
        {5, -1, -5, 0},
        {-5, -1, 5, 0},
        {MIN, MIN, 1, 0},
        {MAX, MIN, -1, -1},
        {MIN, MAX, -2, MAX - 1},
        {-1, MIN, 0, -1},
    };

    for (const auto &c : cases) {
        CAPTURE(c.x);
        CAPTURE(c.y);
        CHECK(math::floorDiv(c.x, c.y) == c.floorDiv);
        CHECK(math::floorMod(c.x, c.y) == c.floorMod);
    }

    CHECK_THROWS_AS(math::floorDiv(1, 0), InvalidArgumentException);
    CHECK_THROWS_AS(math::floorMod(1, 0), InvalidArgumentException);

    static_assert(math::floorMod(MIN, 1000) == 192);
    static_assert(math::floorDiv(MIN, -1) == MIN);
}

TEST_CASE("doubleEquals should match Java Double.equals semantics") {
    CHECK(math::doubleEquals(1.0, 1.0));
    CHECK_FALSE(math::doubleEquals(1.0, std::nextafter(1.0, 2.0)));
    CHECK_FALSE(math::doubleEquals(0.0, -0.0));
    CHECK(math::doubleEquals(std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::infinity()));
    CHECK(math::doubleEquals(nanWithPayload(1), nanWithPayload(2)));
}

TEST_CASE("doubleCompare should match Java Double.compare ordering") {
    const auto infinity = std::numeric_limits<double>::infinity();
    const auto firstNaN = nanWithPayload(1);
    const auto secondNaN = nanWithPayload(2);

    CHECK(math::doubleCompare(-infinity, -1.0) < 0);
    CHECK(math::doubleCompare(-0.0, 0.0) < 0);
    CHECK(math::doubleCompare(0.0, -0.0) > 0);
    CHECK(math::doubleCompare(infinity, firstNaN) < 0);
    CHECK(math::doubleCompare(firstNaN, infinity) > 0);
    CHECK(math::doubleCompare(firstNaN, secondNaN) == 0);
}

TEST_CASE("approximatelyEquals should use explicit scale-aware tolerances") {
    constexpr double relativeTolerance = 1.0e-12;
    constexpr double absoluteTolerance = 0.0;
    const auto infinity = std::numeric_limits<double>::infinity();

    CHECK(math::approximatelyEquals(1.0, std::nextafter(1.0, 2.0), relativeTolerance, absoluteTolerance));
    CHECK(math::approximatelyEquals(infinity, infinity, relativeTolerance, absoluteTolerance));
    CHECK_FALSE(math::approximatelyEquals(0.0, 1.0e-20, relativeTolerance, absoluteTolerance));
    CHECK(math::approximatelyEquals(0.0, 1.0e-20, relativeTolerance, 1.0e-20));
    CHECK_FALSE(math::approximatelyEquals(math::NaN, math::NaN, relativeTolerance, absoluteTolerance));
    CHECK_FALSE(math::approximatelyEquals(1.0, 1.0, -1.0, absoluteTolerance));
    CHECK_FALSE(math::approximatelyEquals(1.0, 1.0, relativeTolerance, infinity));
}

TEST_CASE("Bit shifts should shift in range, give 0 or -1 for the type width or more and reverse a negative shift") {
    constexpr auto MIN = std::numeric_limits<std::int64_t>::min();
    constexpr auto MAX = std::numeric_limits<std::int64_t>::max();

    // In range: the two's complement shifts (sal/shl: <<, sar: >>, shr: >>> in Java).
    CHECK(sal(std::int64_t{1}, 63) == MIN);
    CHECK(sal(std::int64_t{-1}, 1) == -2);
    CHECK(sal(MIN, 1) == 0);
    CHECK(sar(MIN, 63) == -1);
    CHECK(sar(std::int64_t{-8}, 1) == -4);
    CHECK(shr(std::int64_t{-1}, 1) == MAX);
    CHECK(shr(std::int64_t{-1}, 63) == 1);
    CHECK(shl(std::uint64_t{1}, 63) == UINT64_C(0x8000000000000000));

    // Narrow types: the result is truncated to the type of the value.
    CHECK(sal(std::int8_t{0x40}, 1) == std::int8_t{-128});
    CHECK(sar(std::int8_t{-128}, 7) == std::int8_t{-1});
    CHECK(shr(std::int8_t{-1}, 4) == std::int8_t{0x0F});
    CHECK(shl(std::uint16_t{0x8001}, 1) == std::uint16_t{2});
    CHECK(shr(std::int32_t{-1}, 28) == 0xF);

    // The type width or more: 0, or -1 for the arithmetic right shift of a negative value (Java masks the shift).
    CHECK(sal(std::int64_t{1}, 64) == 0);
    CHECK(shl(std::int32_t{-1}, 100) == 0);
    CHECK(sar(std::int64_t{-5}, 64) == -1);
    CHECK(sar(std::int64_t{5}, 1000) == 0);
    CHECK(shr(std::int64_t{-1}, 64) == 0);
    CHECK(shr(std::uint8_t{0xFF}, 8U) == 0);

    // A negative shift shifts to the other side.
    CHECK(sal(std::int64_t{-16}, -2) == -4);
    CHECK(sar(std::int64_t{3}, -2) == 12);
    CHECK(shl(std::int64_t{-1}, -60) == 0xF);
    CHECK(shr(std::int64_t{1}, -63) == MIN);
    CHECK(sal(std::int64_t{1}, -64) == 0);
    CHECK(sar(std::int64_t{-1}, -64) == 0);
}

TEST_CASE("A shift by the minimum of a signed shift type should shift to the other side by its magnitude") {
    // The magnitude (2^31, 2^63) is more than the width, so the result is 0, or -1 for sar of a negative value.
    constexpr auto MIN32 = std::numeric_limits<std::int32_t>::min();
    constexpr auto MIN64 = std::numeric_limits<std::int64_t>::min();

    CHECK(sal(std::int64_t{-1}, MIN32) == -1);
    CHECK(sar(std::int64_t{-1}, MIN32) == 0);
    CHECK(shl(std::int64_t{-1}, MIN32) == 0);
    CHECK(shr(std::int64_t{-1}, MIN32) == 0);
    CHECK(sal(std::int32_t{-1}, MIN64) == -1);
    CHECK(shr(std::int32_t{-1}, MIN64) == 0);
}

TEST_CASE("andOp, orOp and xorOp should use the larger unsigned type and return the type of the first argument") {
    CHECK(andOp(std::int64_t{-1}, std::uint8_t{0xF0}) == 0xF0);
    CHECK(andOp(std::uint8_t{0xFF}, -2) == std::uint8_t{0xFE});
    CHECK(orOp(std::int32_t{0x10}, std::int64_t{0x100000001}) == 0x11);
    CHECK(xorOp(std::int8_t{-1}, 0x0F) == std::int8_t{-16});
    CHECK(orOp(std::int64_t{0}, std::int8_t{-1}) == -1);
}

TEST_CASE("getBits and setBits should read and replace a bit field") {
    CHECK(getBits(std::int32_t{0x12345678}, 0xFF, 8) == 0x56);
    CHECK(getBits(std::int64_t{-1}, 0xFF, 60) == 0xF);
    CHECK(setBits(std::int32_t{0x12345678}, 0xFF, 8, 0xAB) == 0x1234AB78);
    CHECK(setBits(std::int32_t{0x12345678}, 0xFF, 8, 0x1AB) == 0x1234AB78);
    CHECK(setBits(std::int64_t{-1}, 0xFF, 56, 0) == 0x00FFFFFFFFFFFFFF);
    CHECK(setBits(std::uint16_t{0}, std::uint16_t{0x3}, std::uint16_t{14}, std::uint16_t{0x3}) == 0xC000);
}
