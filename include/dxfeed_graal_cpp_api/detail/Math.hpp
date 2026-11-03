// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// This header is not part of the API. It is installed only because public headers need it; its contents may change
// without notice.

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./Concepts.hpp"

#include <climits>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

DXFCPP_BEGIN_NAMESPACE

namespace math {

static const double NaN = std::numeric_limits<double>::quiet_NaN();

// The functions below compare floating-point numbers exactly on purpose.
DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wfloat-equal")
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wfloat-equal")

/**
 * Exact IEEE 754 equality of floating-point numbers, as @c == in Java: NaN is not equal to anything, and positive and
 * negative zero are equal. Use it where an exact comparison is intended (-Wfloat-equal reports the others).
 *
 * @param a The first value.
 * @param b The second value.
 * @return @c true if the values are equal; @c false otherwise.
 */
template <std::floating_point T, std::floating_point U> constexpr bool exactlyEquals(T a, U b) noexcept {
    return a == b;
}

/**
 * Compares two double values using Java @c Double.equals semantics.
 * All NaN values are considered equal, while positive and negative zero are considered different.
 *
 * @param a The first value.
 * @param b The second value.
 * @return @c true if the values are equal; @c false otherwise.
 */
inline bool doubleEquals(double a, double b) noexcept {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }

    if (a != b) {
        return false;
    }

    return a != 0.0 || std::signbit(a) == std::signbit(b);
}

/**
 * Compares two double values using Java @c Double.compare semantics.
 * NaN is greater than every other value and all NaN values compare as equal. Positive zero is greater than negative
 * zero.
 *
 * @param a The first value.
 * @param b The second value.
 * @return A negative value if @p a is less than @p b, zero if they are equal, or a positive value otherwise.
 */
inline std::int32_t doubleCompare(double a, double b) noexcept {
    if (a < b) {
        return -1;
    }

    if (a > b) {
        return 1;
    }

    if (doubleEquals(a, b)) {
        return 0;
    }

    if (std::isnan(a)) {
        return 1;
    }

    if (std::isnan(b)) {
        return -1;
    }

    return std::signbit(a) ? -1 : 1;
}

DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_GCC_WARNINGS_POP()

template <Integral Type, typename ResultType = int> ResultType compare(Type v1, Type v2) {
    if (v1 < v2) {
        return -1;
    }

    if (v2 < v1) {
        return 1;
    }

    return 0;
}

inline int compare(double d1, double d2) {
    if (std::isnan(d1) || std::isnan(d2)) {
        return std::isnan(d1) ? (std::isnan(d2) ? 0 : -1) : 1;
    }

    if (d1 < d2) {
        return -1;
    }

    if (d1 > d2) {
        return 1;
    }

    return 0;
}

} // namespace math

namespace math_util {

/**
 * Returns quotient according to number theory - i.e. when remainder is zero or positive.
 *
 * @tparam T The dividend's and divisor's type
 * @param a dividend
 * @param b divisor
 * @return quotient according to number theory
 */
template <Integral T> constexpr static T div(T a, T b) {
    if (a >= 0) {
        return a / b;
    }

    if (b >= 0) {
        return ((a + 1) / b) - 1;
    }

    return ((a + 1) / b) + 1;
}

/**
 * An int32 value with the arithmetic of the Java `int`: a sum, a difference, a product or a negation wraps around
 * modulo 2^32 instead of overflowing; a quotient and a remainder are those of the signed values.
 */
struct JavaInt {
    /// The value.
    std::int32_t value;

    /**
     * Wraps a value.
     *
     * @param i The value.
     */
    constexpr JavaInt(std::int32_t i) noexcept : value{i} { // NOLINT(*-explicit-constructor)
    }

    /// @return The sum of the values, wrapped around.
    friend constexpr JavaInt operator+(JavaInt a, JavaInt b) noexcept {
        return wrap(static_cast<std::uint32_t>(a.value) + static_cast<std::uint32_t>(b.value));
    }

    /// @return The difference of the values, wrapped around.
    friend constexpr JavaInt operator-(JavaInt a, JavaInt b) noexcept {
        return wrap(static_cast<std::uint32_t>(a.value) - static_cast<std::uint32_t>(b.value));
    }

    /// @return The product of the values, wrapped around.
    friend constexpr JavaInt operator*(JavaInt a, JavaInt b) noexcept {
        return wrap(static_cast<std::uint32_t>(a.value) * static_cast<std::uint32_t>(b.value));
    }

    /// @return The quotient of the values, rounded toward zero (the divisor must be neither 0 nor -1).
    friend constexpr JavaInt operator/(JavaInt a, JavaInt b) noexcept {
        return a.value / b.value;
    }

    /// @return The remainder of the values, with the sign of the dividend.
    friend constexpr JavaInt operator%(JavaInt a, JavaInt b) noexcept {
        return a.value % b.value;
    }

    /// @return The negated value, wrapped around: `-Integer.MIN_VALUE` is `Integer.MIN_VALUE`.
    constexpr JavaInt operator-() const noexcept {
        return wrap(0U - static_cast<std::uint32_t>(value));
    }

    private:
    // C++20 defines the conversion of an unsigned value to a signed one as the wrap around modulo 2^32.
    static constexpr JavaInt wrap(std::uint32_t value) noexcept {
        return static_cast<std::int32_t>(value);
    }
};

} // namespace math_util

namespace day_util {

/**
 * Returns yyyymmdd integer in Gregorian calendar for a specified day identifier.
 * The day identifier is defined as the number of days since Unix epoch of January 1, 1970.
 * The result is equal to `yearSign * (abs(year) * 10000 + month * 100 + day)`, where year,
 * month, and day are in Gregorian calendar, month is between 1 and 12 inclusive, and day is counted from 1.
 *
 * Any day id gives the result of Java: the arithmetic wraps around as that of the Java `int`.
 *
 * @param dayId The day id
 * @return integer date
 */
constexpr static std::int32_t getYearMonthDayByDayId(std::int32_t dayId) {
    using math_util::JavaInt;

    JavaInt j = JavaInt{dayId} + 2472632; // this shifts the epoch back to astronomical year -4800
    JavaInt g = math_util::div(j.value, 146097);
    JavaInt dg = j - g * 146097;
    JavaInt c = (dg / 36524 + 1) * 3 / 4;
    JavaInt dc = dg - c * 36524;
    JavaInt b = dc / 1461;
    JavaInt db = dc - b * 1461;
    JavaInt a = (db / 365 + 1) * 3 / 4;
    JavaInt da = db - a * 365;
    // this is the integer number of full years elapsed since March 1, 4801 BC at 00:00 UTC
    JavaInt y = g * 400 + c * 100 + b * 4 + a;
    // this is the integer number of full months elapsed since the last March 1 at 00:00 UTC
    JavaInt m = (da * 5 + 308) / 153 - 2;
    // this is the number of days elapsed since day 1 of the month at 00:00 UTC
    JavaInt d = da - (m + 4) * 153 / 5 + 122;
    JavaInt yyyy = y - 4800 + (m + 2) / 12;
    JavaInt mm = (m + 2) % 12 + 1;
    JavaInt dd = d + 1;
    // Math.abs(Integer.MIN_VALUE) is Integer.MIN_VALUE
    JavaInt yyyymmdd = (yyyy.value < 0 ? -yyyy : yyyy) * 10000 + mm * 100 + dd;

    return yyyy.value >= 0 ? yyyymmdd.value : (-yyyymmdd).value;
}

} // namespace day_util

template <std::size_t Bits> struct hashMixImpl;

template <> struct hashMixImpl<64> {
    constexpr static std::uint64_t fn(std::uint64_t x) noexcept {
        std::uint64_t const m = (static_cast<std::uint64_t>(0xe9846af) << 32) + 0x9b1a615d;

        x ^= x >> 32;
        x *= m;
        x ^= x >> 32;
        x *= m;
        x ^= x >> 28;

        return x;
    }
};

template <> struct hashMixImpl<32> {
    constexpr static std::uint32_t fn(std::uint32_t x) noexcept {
        std::uint32_t const m1 = 0x21f0aaad;
        std::uint32_t const m2 = 0x735a2d97;

        x ^= x >> 16;
        x *= m1;
        x ^= x >> 15;
        x *= m2;
        x ^= x >> 15;

        return x;
    }
};

constexpr static std::size_t hashMix(std::size_t v) noexcept {
    return hashMixImpl<sizeof(std::size_t) * CHAR_BIT>::fn(v);
}

template <class T> constexpr void hashCombine(std::size_t &seed, const T &v) noexcept {
    seed = hashMix(seed + 0x9e3779b9 + std::hash<T>()(v));
}

template <typename T, typename U> T fitToType(const U &size) {
    static_assert(sizeof(T) <= sizeof(U));

    return static_cast<T>(static_cast<U>((std::numeric_limits<T>::max)()) < size ? (std::numeric_limits<T>::max)()
                                                                                 : size);
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
