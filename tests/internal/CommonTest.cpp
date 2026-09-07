// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/internal/Common.hpp>

#include <bit>
#include <cstdint>
#include <limits>

using namespace dxfcpp;

namespace {

double nanWithPayload(std::uint64_t payload) {
    return std::bit_cast<double>(UINT64_C(0x7ff8000000000000) | payload);
}

template <typename T, typename U>
concept ApproximatelyComparable = requires(T a, U b) { math::approximatelyEquals(a, b, 1.0e-12, 0.0); };

static_assert(ApproximatelyComparable<float, double>);
static_assert(!ApproximatelyComparable<std::int64_t, std::int64_t>);

} // namespace

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
