// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include "utils/StringUtils.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif
#include <climits>
#include <cstring>

#include <charconv>
#include <chrono>
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "utils/debug/Debug.hpp"

DXFCPP_BEGIN_NAMESPACE

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept EnumConcept = std::is_enum_v<T>;

template <class From, class To>
concept ConvertibleTo = std::is_convertible_v<From, To> && requires { static_cast<To>(std::declval<From>()); };

namespace detail {
template <typename T>
struct RemoveAllPointers
    : std::conditional_t<std::is_pointer_v<T>, RemoveAllPointers<std::remove_pointer_t<T>>, std::type_identity<T>> {};
} // namespace detail

template <typename T> using RemoveAllPointers = typename detail::RemoveAllPointers<T>::type;

struct DXFeedEventListener {};

struct DXEndpointStateChangeListener {};

struct IpfPropertyChangeListener {};

struct InstrumentProfileUpdateListener {};

template <typename... T> constexpr void ignore_unused(const T &...) {
}

constexpr inline auto is_constant_evaluated(bool default_value = false) noexcept -> bool {
#ifdef __cpp_lib_is_constant_evaluated
    ignore_unused(default_value);
    return std::is_constant_evaluated();
#else
    return default_value;
#endif
}

// Implementation of std::bit_cast for pre-C++20.
template <typename To, typename From>
constexpr To bit_cast(const From &from)
#if __cpp_concepts
    requires(sizeof(To) == sizeof(From))
#endif
{
#ifdef __cpp_lib_bit_cast
    if (is_constant_evaluated())
        return std::bit_cast<To>(from);
#endif
    auto to = To();
    // The cast suppresses a bogus -Wclass-memaccess on GCC.
    std::memcpy(static_cast<void *>(&to), &from, sizeof(to));
    return to;
}

/// Lightweight implementation of "nullable bool"
enum class Tristate : std::uint8_t {
    FALSE = 0,
    TRUE = 1,
    NONE = 2,
};

/**
 * @return The number of milliseconds that have passed since the start of the Unix epoch (1970-01-01).
 */
inline auto now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

namespace math {
static constexpr std::int64_t floorDiv(std::int64_t x, std::int64_t y) {
    std::int64_t r = x / y;

    // if the signs are different and modulo not zero, round down
    if ((x < 0) != (y < 0) && (r * y != x)) {
        r--;
    }

    return r;
}

static constexpr std::int64_t floorMod(std::int64_t x, std::int64_t y) {
    return x - (floorDiv(x, y) * y);
}

static const double NaN = std::numeric_limits<double>::quiet_NaN();

static inline bool equals(double a, double b, double eps = std::numeric_limits<double>::epsilon()) {
    if (std::isnan(a) || std::isnan(b)) {
        return false;
    }

    return std::abs(a - b) < eps;
}

template <typename T, typename U>
static inline bool equals(T a, U b, double eps = std::numeric_limits<double>::epsilon()) {
    if (std::isnan(static_cast<double>(a)) || std::isnan(static_cast<double>(b))) {
        return false;
    }

    return std::abs(static_cast<double>(a) - static_cast<double>(b)) < eps;
}

} // namespace math

namespace time_nanos_util {
static constexpr std::int64_t NANOS_IN_MILLIS = 1'000'000LL;

/**
 * Returns time measured in nanoseconds since epoch from the time in milliseconds and its nano part.
 * The result of this method is `timeMillis * 1'000'000 + timeNanoPart`.
 *
 * @param timeMillis time in milliseconds since epoch.
 * @param timeNanoPart nanoseconds part that shall lie within [0..999999] interval.
 * @return time measured in nanoseconds since epoch.
 */
static constexpr std::int64_t getNanosFromMillisAndNanoPart(std::int64_t timeMillis, std::int32_t timeNanoPart) {
    return (timeMillis * NANOS_IN_MILLIS) + timeNanoPart;
}

/**
 * Returns time measured in milliseconds since Java epoch from the time in nanoseconds.
 * Idea is that nano part of time shall be within [0..999999] interval
 * so that the following equation always holds
 * `getMillisFromNanos(timeNanos) * 1'000'000 + getNanoPartFromNanos(timeNanos) == timeNanos`.
 *
 * @param timeNanos time measured in nanoseconds since epoch
 * @return time measured in milliseconds since epoch.
 * @see ::getNanoPartFromNanos()
 */
static constexpr std::int64_t getMillisFromNanos(std::int64_t timeNanos) {
    return math::floorDiv(timeNanos, NANOS_IN_MILLIS);
}

/**
 * Returns nano part of time.
 * Idea is that nano part of time shall be within [0..999999] interval
 * so that the following equation always holds
 * `getMillisFromNanos(timeNanos) * 1'000'000 + getNanoPartFromNanos(timeNanos) == timeNanos`.
 *
 * @param timeNanos time measured in nanoseconds since epoch
 * @return time measured in milliseconds since epoch.
 * @see ::getMillisFromNanos()
 */
static constexpr std::int32_t getNanoPartFromNanos(std::int64_t timeNanos) {
    return static_cast<std::int32_t>(math::floorMod(timeNanos, NANOS_IN_MILLIS));
}

} // namespace time_nanos_util

namespace time_util {
/// Number of milliseconds in a second.
static constexpr std::int64_t SECOND = 1000LL;

/// Number of milliseconds in a minute.
static constexpr std::int64_t MINUTE = 60LL * SECOND;

/// Number of milliseconds in an hour.
static constexpr std::int64_t HOUR = 60LL * MINUTE;

/// Number of milliseconds in an day.
static constexpr std::int64_t DAY = 24LL * HOUR;

/**
 * Returns correct number of milliseconds with proper handling negative values.
 * Idea is that number of milliseconds shall be within [0..999] interval
 *
 * @param timeMillis The timestamp in milliseconds
 * @return a correct number of milliseconds
 */
static std::int32_t getMillisFromTime(std::int64_t timeMillis) {
    return static_cast<std::int32_t>(math::floorMod(timeMillis, SECOND));
}

/**
 * Returns correct number of seconds with proper handling negative values and overflows.
 * Idea is that number of milliseconds shall be within [0..999] interval
 *
 * @param timeMillis The timestamp in milliseconds
 * @return a correct number of second
 */
static constexpr std::int32_t getSecondsFromTime(std::int64_t timeMillis) {
    if (timeMillis >= 0) {
        return static_cast<std::int32_t>(
            std::min(timeMillis / SECOND, static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
    }

    return static_cast<std::int32_t>(
        std::max((timeMillis + 1) / SECOND - 1, static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())));
}
} // namespace time_util

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

template <Integral T> constexpr static T abs(T a) {
    return a < 0 ? -a : a;
}

} // namespace math_util

namespace day_util {

static std::int32_t DAY_OF_YEAR[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

/**
 * Returns yyyymmdd integer in Gregorian calendar for a specified day identifier.
 * The day identifier is defined as the number of days since Unix epoch of January 1, 1970.
 * The result is equal to `yearSign * (abs(year) * 10000 + month * 100 + day)`, where year,
 * month, and day are in Gregorian calendar, month is between 1 and 12 inclusive, and day is counted from 1.
 *
 * @param dayId The day id
 * @return integer date
 */
constexpr static std::int32_t getYearMonthDayByDayId(std::int32_t dayId) {
    std::int32_t j = dayId + 2472632; // this shifts the epoch back to astronomical year -4800
    std::int32_t g = math_util::div(j, 146097);
    std::int32_t dg = j - g * 146097;
    std::int32_t c = (dg / 36524 + 1) * 3 / 4;
    std::int32_t dc = dg - c * 36524;
    std::int32_t b = dc / 1461;
    std::int32_t db = dc - b * 1461;
    std::int32_t a = (db / 365 + 1) * 3 / 4;
    std::int32_t da = db - a * 365;
    std::int32_t y = g * 400 + c * 100 + b * 4 +
                     a; // this is the integer number of full years elapsed since March 1, 4801 BC at 00:00 UTC
    std::int32_t m = (da * 5 + 308) / 153 -
                     2; // this is the integer number of full months elapsed since the last March 1 at 00:00 UTC
    std::int32_t d =
        da - (m + 4) * 153 / 5 + 122; // this is the number of days elapsed since day 1 of the month at 00:00 UTC
    std::int32_t yyyy = y - 4800 + (m + 2) / 12;
    std::int32_t mm = (m + 2) % 12 + 1;
    std::int32_t dd = d + 1;
    std::int32_t yyyymmdd = math_util::abs(yyyy) * 10000 + mm * 100 + dd;

    return yyyy >= 0 ? yyyymmdd : -yyyymmdd;
}

static std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day);

} // namespace day_util

namespace detail {
template <typename...> struct MaxImpl;

template <typename T> struct MaxImpl<T> {
    using Type = T;
};

template <typename T, typename U> struct MaxImpl<T, U> {
    using Type = std::conditional_t<sizeof(T) >= sizeof(U), T, U>;
};

template <typename T, typename U, typename V, typename... Ws> struct MaxImpl<T, U, V, Ws...> {
    using Type = typename MaxImpl<T, typename MaxImpl<U, typename MaxImpl<V, Ws...>::Type>::Type>::Type;
};
} // namespace detail

/**
 * Returns max type by size (first is better)
 */
template <typename... Ts> using Max = typename detail::MaxImpl<Ts...>::Type;

/**
 * Performs a right arithmetic bit shift operation (>> in Java, C, etc). The sign bit is extended to preserve the
 * signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then if the `value` is
 * negative (signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V sar(V value, S shift) noexcept;

/**
 * Performs a left arithmetic bit shift operation (<< in Java, C, etc).
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sar() "right arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V leftArithmeticShift(V value, S shift) noexcept {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return sar(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    auto unsignedShift = static_cast<std::make_unsigned_t<S>>(shift);

    if (unsignedShift >= sizeof(V) * CHAR_BIT) {
        return 0;
    }

    return value << unsignedShift;
}

/**
 * Performs a left arithmetic bit shift operation (<< in Java, C, etc).
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sar() "right arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V sal(V value, S shift) noexcept {
    return leftArithmeticShift(value, shift);
}

/**
 * Performs a right arithmetic bit shift operation (>> in Java, C, etc). The sign bit is extended to preserve the
 * signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then if the `value` is
 * negative (signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V rightArithmeticShift(V value, S shift) noexcept {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return sal(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    auto unsignedShift = static_cast<std::make_unsigned_t<S>>(shift);

    if (unsignedShift >= sizeof(V) * CHAR_BIT) {
        return value < 0 ? -1 : 0;
    }

    return value >> unsignedShift;
}

/**
 * Performs a right arithmetic bit shift operation (>> in Java, C, etc). The sign bit is extended to preserve the
 * signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then if the `value` is
 * negative (signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V sar(V value, S shift) noexcept {
    return rightArithmeticShift(value, shift);
}

/**
 * Performs a right logical bit shift operation (>>> in Java). Fills the left bits by zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V shr(V value, S shift) noexcept;

/**
 * Performs a left logical bit shift operation.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shr() "right logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V leftLogicalShift(V value, S shift) noexcept {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return shr(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    auto unsignedShift = static_cast<std::make_unsigned_t<S>>(shift);

    if (unsignedShift >= sizeof(V) * CHAR_BIT) {
        return 0;
    }

    return value << unsignedShift;
}

/**
 * Performs a left logical bit shift operation.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shr() "right logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V shl(V value, S shift) noexcept {
    return leftLogicalShift(value, shift);
}

/**
 * Performs a right logical bit shift operation (>>> in Java). Fills the left bits by zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V rightLogicalShift(V value, S shift) noexcept {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return shl(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    auto unsignedShift = static_cast<std::make_unsigned_t<S>>(shift);

    if (unsignedShift >= sizeof(V) * CHAR_BIT) {
        return 0;
    }

    return static_cast<V>(static_cast<std::make_unsigned_t<V>>(value) >> unsignedShift);
}

/**
 * Performs a right logical bit shift operation (>>> in Java). Fills the left bits by zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam V The type of `value`
 * @tparam S The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <Integral V, Integral S> static constexpr V shr(V value, S shift) noexcept {
    return rightLogicalShift(value, shift);
}

template <Integral A, Integral B> static constexpr A andOp(A a, B b) noexcept {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) & static_cast<Common>(b));
}

template <Integral A, Integral B> static constexpr A orOp(A a, B b) noexcept {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) | static_cast<Common>(b));
}

template <Integral A, Integral B> static constexpr A xorOp(A a, B b) noexcept {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) ^ static_cast<Common>(b));
}

template <Integral F, Integral M, Integral S> static constexpr F getBits(F flags, M mask, S shift) noexcept {
    return static_cast<F>(andOp(shr(flags, shift), mask));
}

template <Integral T> static constexpr T setBits(T flags, T mask, T shift, T bits) noexcept {
    if constexpr (std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;

        return static_cast<T>((static_cast<U>(flags) & ~(static_cast<U>(mask) << static_cast<U>(shift))) |
                              ((static_cast<U>(bits) & static_cast<U>(mask)) << static_cast<U>(shift)));
    } else {
        return (flags & ~(mask << shift)) | ((bits & mask) << shift);
    }
}

template <Integral F, Integral M, Integral S, Integral B>
static constexpr F setBits(F flags, M mask, S shift, B bits) noexcept {
    if constexpr (std::is_signed_v<F> || std::is_signed_v<M> || std::is_signed_v<S> || std::is_signed_v<B>) {
        using U = std::make_unsigned_t<Max<F, M, S, B>>;

        return static_cast<F>((static_cast<U>(flags) & ~(static_cast<U>(mask) << static_cast<U>(shift))) |
                              ((static_cast<U>(bits) & static_cast<U>(mask)) << static_cast<U>(shift)));
    } else {
        return (flags & ~(mask << shift)) | ((bits & mask) << shift);
    }
}

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

template <Integral T, Integral U> constexpr std::size_t pack(T t, U u) noexcept {
    constexpr auto sizeOfSize = sizeof(std::size_t) * CHAR_BIT;

    return orOp(shl(t, sizeOfSize / 2), u);
}

constexpr std::pair<std::size_t, std::size_t> unpack(std::size_t v) noexcept {
    constexpr auto sizeOfSize = sizeof(std::size_t) * CHAR_BIT;

    return {shr(v, sizeOfSize / 2), andOp(v, shr(~std::size_t{}, sizeOfSize / 2))};
}

template <typename T, typename U> T fitToType(const U &size) {
    static_assert(sizeof(T) <= sizeof(U));

    return static_cast<T>(static_cast<U>(std::numeric_limits<T>::max()) < size ? std::numeric_limits<T>::max() : size);
}

/**
 * A simple wrapper around strings or something similar to strings to reduce the amount of code for methods that take
 * strings as input.
 */
struct StringLikeWrapper {
    using DataType = std::variant<std::string, std::string_view>;

  private:
    DataType data_{};

  public:
    StringLikeWrapper(std::string_view sv) : data_{sv} {
    }

    StringLikeWrapper(const char *chars) : data_{chars == nullptr ? std::string_view{} : std::string_view{chars}} {
    }

    StringLikeWrapper(const std::string &s) : data_{s} {
    }

    StringLikeWrapper(std::string &&s) : data_{std::move(s)} {
    }

    template <auto N>
    StringLikeWrapper(const char (&chars)[N]) : StringLikeWrapper{std::string_view{chars, chars + N}} {
    }

    operator std::string() const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return {sv->data(), sv->size()};
        } else {
            return std::get<std::string>(data_);
        }
    }

    operator std::string_view() const & {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return *sv;
        } else {
            return std::get<std::string>(data_);
        }
    }

    const char *data() const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return sv->data();
        } else {
            return std::get<std::string>(data_).c_str();
        }
    }

    const char *c_str() const {
        return data();
    }

    bool empty() const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return sv->empty();
        } else {
            return std::get<std::string>(data_).empty();
        }
    }

    std::size_t size() const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return sv->size();
        } else {
            return std::get<std::string>(data_).size();
        }
    }

    std::size_t length() const {
        return size();
    }

    bool ends_with(const StringLikeWrapper &sw) const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            return sv->ends_with(sw);
        } else {
            return std::get<std::string>(data_).ends_with(sw);
        }
    }

    std::string substr(std::string::size_type pos = 0, std::string::size_type count = std::string::npos) const {
        if (auto sv = std::get_if<std::string_view>(&data_); sv) {
            auto sv2 = sv->substr(pos, count);

            return {sv2.data(), sv2.size()};
        } else {
            return std::get<std::string>(data_).substr(pos, count);
        }
    }

    bool operator==(const StringLikeWrapper &sw) const {
        return sw.operator std::string_view() == this->operator std::string_view();
    }

    friend std::string operator+(const StringLikeWrapper &sw1, const StringLikeWrapper &sw2) {
        return sw1.operator std::string() + sw2.operator std::string();
    }

    explicit operator double() const {
        double result{};

        // At the moment, clang\apple clang's std lib does not support a version of the `from_chars` function (needed
        // for `std::string_view`) for the `double` type.
#ifdef _LIBCPP_VERSION
        auto s = this->operator std::string();

        result = std::stod(s);
#else
        auto sw = this->operator std::string_view();

        std::from_chars(sw.data(), sw.data() + sw.size(), result);
#endif

        return result;
    }
};

struct StringHash {
    using HashType = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char *str) const {
        return HashType{}(str);
    }

    std::size_t operator()(std::string_view str) const {
        return HashType{}(str);
    }

    std::size_t operator()(std::string const &str) const {
        return HashType{}(str);
    }

    std::size_t operator()(const StringLikeWrapper &sw) const {
        return HashType{}(sw);
    }
};

namespace util {

inline void throwInvalidChar(char c, const std::string &name);

inline void checkChar(char c, std::uint32_t mask, const std::string &name) {
    if ((andOp(c, ~mask)) != 0) {
        throwInvalidChar(c, name);
    }
}
} // namespace util

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
