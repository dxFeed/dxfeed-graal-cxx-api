// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif
#include <cstring>

#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <sstream>
#include <type_traits>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {
template <typename T>
concept Integral = std::is_integral_v<T>;

struct DXFeedEventListener {};

struct DXEndpointStateChangeListener {};

#if defined(NDEBUG) && !defined(DXFCPP_DEBUG)
constexpr bool isDebug = false;
constexpr bool isDebugIsolates = false;
#else
constexpr bool isDebug = true;

#    ifdef DXFCPP_DEBUG_ISOLATES
constexpr bool isDebugIsolates = true;
#    else
constexpr bool isDebugIsolates = false;
#    endif

#endif

#if defined(__clang__)
constexpr bool isClangFlavouredCompiler = true;
#else
constexpr bool isClangFlavouredCompiler = false;
#endif

template <typename... T> constexpr void ignore_unused(const T &...) {}

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
    requires(sizeof(To) == sizeof(From))
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

// final_action allows you to ensure something gets run at the end of a scope
template <class F> class final_action {
  public:
    explicit final_action(const F &ff) noexcept : f{ff} {}
    explicit final_action(F &&ff) noexcept : f{std::move(ff)} {}

    ~final_action() noexcept {
        if (invoke)
            f();
    }

    final_action(final_action &&other) noexcept : f(std::move(other.f)), invoke(std::exchange(other.invoke, false)) {}

    final_action(const final_action &) = delete;
    void operator=(const final_action &) = delete;
    void operator=(final_action &&) = delete;

  private:
    F f;
    bool invoke = true;
};

// finally() - convenience function to generate a final_action
template <class F> [[nodiscard]] auto finally(F &&f) noexcept {
    return final_action<std::decay_t<F>>{std::forward<F>(f)};
}

inline auto now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

template <typename... Args> std::string vformat(std::string_view format, Args &&...args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}

template <typename... Args> void vprint(std::ostream &os, std::string_view format, Args &&...args) {
    fmt::vprint(os, format, fmt::make_format_args(args...));
}

inline std::string nowStr() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d %H%M%S}.{:0>3}", std::chrono::floor<std::chrono::seconds>(now), ms);
}

inline std::string nowStrWithTimeZone() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", std::chrono::floor<std::chrono::seconds>(now), ms,
                       std::chrono::floor<std::chrono::seconds>(now));
}

inline std::string formatTimeStamp(std::int64_t timestamp) {
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%y%m%d-%H%M%S%z}", tm);
}

inline std::string formatTimeStampWithMillis(std::int64_t timestamp) {
    auto ms = timestamp % 1000;
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", tm, ms, tm);
}

inline std::string debugPrefixStr() {
    std::ostringstream tid{};

    tid << std::this_thread::get_id();

    return fmt::format("D {} [{}]", nowStr(), tid.str());
}

template <typename... Args> inline void debug(std::string_view format, Args &&...args) {
    vprint(std::cerr, "{} {}\n", debugPrefixStr(), vformat(format, std::forward<Args>(args)...));
}

namespace handler_utils {

template <typename T> struct JavaObjectHandler {
    using Type = T;
    static void deleter(void *handler) noexcept;
    explicit JavaObjectHandler(void *handler = nullptr) noexcept : impl_{handler, &deleter} {}

    JavaObjectHandler(JavaObjectHandler &&) = default;
    JavaObjectHandler &operator=(JavaObjectHandler &&) = default;
    virtual ~JavaObjectHandler() noexcept = default;

    [[nodiscard]] std::string toString() const noexcept {
        if (impl_)
            return fmt::format("{}", impl_.get());
        else
            return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept { return impl_.get(); }

    explicit operator bool() const noexcept { return static_cast<bool>(impl_); }

  private:
    std::unique_ptr<void, decltype(&deleter)> impl_;
};

struct EventClassList {
    static std::unique_ptr<EventClassList> create(std::size_t size) noexcept;

    void set(std::size_t index, std::uint32_t id) noexcept;

    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    void *getHandler() noexcept;

    ~EventClassList() noexcept;

  private:
    EventClassList() noexcept;

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace handler_utils

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += fmt::format("'{}'{}", it->getName(), std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename M, typename F, typename... Args> inline void callWithLock(M &mtx, F &&f, Args &&...args) noexcept {
    std::once_flag once{};

    try {
        std::lock_guard guard{mtx};

        return std::call_once(once, std::forward<F>(f), std::forward<Args>(args)...);
    } catch (...) {
        // TODO: error handling
    }
}

template <typename M, typename F, typename... Args>
inline void tryCallWithLock(M &mtx, F &&f, Args &&...args) noexcept {
    std::once_flag once{};

    try {
        std::lock_guard guard{mtx};

        return std::call_once(once, std::forward<F>(f), std::forward<Args>(args)...);
    } catch (...) {
        return std::call_once(once, std::forward<F>(f), std::forward<Args>(args)...);
    }
}

template <typename Collection, typename ElementType>
concept ElementTypeIs = std::is_same_v<std::decay_t<decltype(*std::begin(Collection{}))>, ElementType>;

namespace math {
static constexpr std::int64_t floorDiv(std::int64_t x, std::int64_t y) {
    std::int64_t r = x / y;

    // if the signs are different and modulo not zero, round down
    if ((x < 0) != (y < 0) && (r * y != x)) {
        r--;
    }

    return r;
}

static constexpr std::int64_t floorMod(std::int64_t x, std::int64_t y) { return x - (floorDiv(x, y) * y); }

static const double NaN = std::numeric_limits<double>::quiet_NaN();

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
static const std::int64_t SECOND = 1000LL;

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

namespace string_util {
static std::string encodeChar(std::int16_t c) {
    if (c >= 32 && c <= 126) {
        return std::string{} + static_cast<char>(c);
    }

    if (c == 0) {
        return "\\0";
    }

    return fmt::format("\\u{:04x}", c);
}

static std::string encodeChar(char c) { return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c))); }

} // namespace string_util

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

template <Integral T> constexpr static T abs(T a) { return a < 0 ? -a : a; }

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

constexpr static std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day) {
    // TODO: error handling
    if (month < 1 || month > 12) {
        return -1;
    }

    std::int32_t dayOfYear = DAY_OF_YEAR[month] + day - 1;

    if (month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        dayOfYear++;
    }

    return year * 365 + math_util::div(year - 1, 4) - math_util::div(year - 1, 100) + math_util::div(year - 1, 400) +
           dayOfYear - 719527;
}

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
 * Performs a right arithmetic bit shift operation.
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
template <Integral V, Integral S> static constexpr V sar(V value, S shift);

/**
 * Performs a left arithmetic bit shift operation.
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
template <Integral V, Integral S> static constexpr V leftArithmeticShift(V value, S shift) {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return sar(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    if (shift >= sizeof(V) * 8) {
        return 0;
    }

    return value << static_cast<std::make_unsigned_t<S>>(shift);
}

/**
 * Performs a left arithmetic bit shift operation.
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
template <Integral V, Integral S> static constexpr V sal(V value, S shift) { return leftArithmeticShift(value, shift); }

/**
 * Performs a right arithmetic bit shift operation.
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
template <Integral V, Integral S> static constexpr V rightArithmeticShift(V value, S shift) {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return sal(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    if (shift >= sizeof(V) * 8) {
        if (value < 0) {
            return -1;
        } else {
            return 0;
        }
    }

    return value >> static_cast<std::make_unsigned_t<S>>(shift);
}

/**
 * Performs a right arithmetic bit shift operation.
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
template <Integral V, Integral S> static constexpr V sar(V value, S shift) {
    return rightArithmeticShift(value, shift);
}

/**
 * Performs a right logical bit shift operation.
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
template <Integral V, Integral S> static constexpr V shr(V value, S shift);

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
template <Integral V, Integral S> static constexpr V leftLogicalShift(V value, S shift) {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return shr(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    if (shift >= sizeof(V) * 8) {
        return 0;
    }

    return value << static_cast<std::make_unsigned_t<S>>(shift);
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
template <Integral V, Integral S> static constexpr V shl(V value, S shift) { return leftLogicalShift(value, shift); }

/**
 * Performs a right logical bit shift operation.
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
template <Integral V, Integral S> static constexpr V rightLogicalShift(V value, S shift) {
    if constexpr (std::is_signed_v<S>) {
        if (shift < 0) {
            return shl(value, -shift);
        }
    }

    if (shift == 0 || value == 0) {
        return value;
    }

    if (shift >= sizeof(V) * 8) {
        return 0;
    }

    return static_cast<V>(static_cast<std::make_unsigned_t<V>>(value) >> static_cast<std::make_unsigned_t<S>>(shift));
}

/**
 * Performs a right logical bit shift operation.
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
template <Integral V, Integral S> static constexpr V shr(V value, S shift) { return rightLogicalShift(value, shift); }

template <Integral A, Integral B> static constexpr A andOp(A a, B b) {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) & static_cast<Common>(b));
}

template <Integral A, Integral B> static constexpr A orOp(A a, B b) {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) | static_cast<Common>(b));
}

template <Integral A, Integral B> static constexpr A xorOp(A a, B b) {
    using Common = std::make_unsigned_t<Max<A, B>>;

    return static_cast<A>(static_cast<Common>(a) ^ static_cast<Common>(b));
}

template <Integral F, Integral M, Integral S> static constexpr F getBits(F flags, M mask, S shift) {
    return static_cast<F>(andOp(shr(flags, shift), mask));
}

template <Integral T> static constexpr T setBits(T flags, T mask, T shift, T bits) {
    if constexpr (std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;

        return static_cast<T>((static_cast<U>(flags) & ~(static_cast<U>(mask) << static_cast<U>(shift))) |
                              ((static_cast<U>(bits) & static_cast<U>(mask)) << static_cast<U>(shift)));
    } else {
        return (flags & ~(mask << shift)) | ((bits & mask) << shift);
    }
}

template <Integral F, Integral M, Integral S, Integral B> static constexpr F setBits(F flags, M mask, S shift, B bits) {
    if constexpr (std::is_signed_v<F> || std::is_signed_v<M> || std::is_signed_v<S> || std::is_signed_v<B>) {
        using U = std::make_unsigned_t<Max<F, M, S, B>>;

        return static_cast<F>((static_cast<U>(flags) & ~(static_cast<U>(mask) << static_cast<U>(shift))) |
                              ((static_cast<U>(bits) & static_cast<U>(mask)) << static_cast<U>(shift)));
    } else {
        return (flags & ~(mask << shift)) | ((bits & mask) << shift);
    }
}

std::string toString(const char *chars);

char utf16to8(std::int16_t in);

std::int16_t utf8to16(char in);

struct String {
    inline static constexpr std::string EMPTY = "";
};

} // namespace dxfcpp
