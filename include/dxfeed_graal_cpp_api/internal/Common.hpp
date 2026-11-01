// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

#include "../detail/Concepts.hpp"
#include "../detail/JavaObjectHandle.hpp"
#include "../detail/Math.hpp"
#include "../util/Time.hpp"
#include "../util/Tristate.hpp"

#include "../exceptions/InvalidArgumentException.hpp"
#include "./utils/StringUtils.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif
#include <algorithm>
#include <climits>
#include <concepts>
#include <cstring>

#include <charconv>
#include <chrono>
#include <cmath>
#include <limits>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <bits/bits.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace detail {
template <typename, template <class...> class> struct IsInstanceImpl : std::false_type {};

template <template <class...> class C, typename... Ts> struct IsInstanceImpl<C<Ts...>, C> : std::true_type {};
} // namespace detail

template <typename T, template <class...> class C> constexpr bool isInstanceOf = detail::IsInstanceImpl<T, C>::value;

namespace detail {
template <typename T>
struct RemoveAllPointers
    : std::conditional_t<std::is_pointer_v<T>, RemoveAllPointers<std::remove_pointer_t<T>>, std::type_identity<T>> {};
} // namespace detail

template <typename T> using RemoveAllPointers = typename detail::RemoveAllPointers<T>::type;

template <class... Ts> struct Overloads : Ts... {
    using Ts::operator()...;
};

template <class... Ts> Overloads(Ts...) -> Overloads<Ts...>;

template <typename... T> constexpr void ignoreUnused(const T &...) {
}

constexpr inline auto is_constant_evaluated(bool default_value = false) noexcept -> bool {
#ifdef __cpp_lib_is_constant_evaluated
    ignoreUnused(default_value);
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

#define DXFCPP_MACRO_CONCAT(a, b) DXFCPP_MACRO_CONCAT_INNER(a, b)
#define DXFCPP_MACRO_CONCAT_INNER(a, b) a##b
#define DXFCPP_MACRO_UNIQUE_NAME(base) DXFCPP_MACRO_CONCAT(base, __LINE__)

namespace detail {
// Reports the exception being handled, thrown by the body of DXFCPP_FINALLY, to ErrorManager (not while the library is
// shutting down: the SDK is not called any more then).
DXFCPP_EXPORT void reportScopeExitException(const std::source_location &location) noexcept;

template <typename Func> struct OnScopeExit {
    Func func;
    std::source_location location;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    OnScopeExit(const Func &f, // NOLINT(*-explicit-constructor)
                const std::source_location &sourceLocation = std::source_location::current())
        : func(static_cast<Func &&>(f)), location{sourceLocation} {
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    OnScopeExit(Func &&f, // NOLINT(*-explicit-constructor)
                const std::source_location &sourceLocation = std::source_location::current())
        : func(f), location{sourceLocation} {
    }

    OnScopeExit(const OnScopeExit &) = delete;

    // The body runs in a destructor: an exception of it goes to ErrorManager instead of terminating the process.
    ~OnScopeExit() noexcept {
        try {
            func();
        } catch (...) {
            reportScopeExitException(location);
        }
    }
};
} // namespace detail

#define DXFCPP_FINALLY(...) detail::OnScopeExit DXFCPP_MACRO_UNIQUE_NAME(ose__) = __VA_ARGS__

template <typename GraalList, typename ElementWrapper> struct GraalListUtils {
    static std::ptrdiff_t calculateSize(std::ptrdiff_t initSize) noexcept {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);

        if (initSize < 0) {
            return 0;
        }

        if (initSize > (std::numeric_limits<SizeType>::max)()) {
            return (std::numeric_limits<SizeType>::max)();
        }

        return initSize;
    }

    static void *newList(std::ptrdiff_t size) {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);
        using ElementType = std::remove_pointer_t<decltype(ListType::elements)>;

        auto *list = new ListType{static_cast<SizeType>(size), nullptr};

        if (size == 0) {
            return static_cast<void *>(list);
        }

        list->elements = new ElementType[static_cast<std::size_t>(size)]{nullptr};

        return list;
    }

    static bool setElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);
        using ElementType = std::remove_pointer_t<decltype(ListType::elements)>;

        if (graalList == nullptr || elementIdx < 0 || elementIdx >= (std::numeric_limits<SizeType>::max)() ||
            element == nullptr) {
            return false;
        }

        static_cast<ListType *>(graalList)->elements[elementIdx] = static_cast<ElementType>(element);

        return true;
    }

    static bool freeElements(void *graalList, std::ptrdiff_t count) {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);

        if (graalList == nullptr || count < 0 || count >= (std::numeric_limits<SizeType>::max)()) {
            return false;
        }

        auto *list = static_cast<ListType *>(graalList);

        for (SizeType i = 0; i < count; i++) {
            if (list->elements[i]) {
                ElementWrapper::freeGraal(static_cast<void *>(list->elements[i]));
            }
        }

        delete[] list->elements;
        delete list;

        return true;
    }

    static void freeList(void *graalList) {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);

        if (graalList == nullptr) {
            return;
        }

        auto list = static_cast<ListType *>(graalList);

        if (list->size > 0 && list->elements != nullptr) {
            for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
                if (list->elements[elementIndex]) {
                    ElementWrapper::freeGraal(static_cast<void *>(list->elements[elementIndex]));
                }
            }

            delete[] list->elements;
        }

        delete list;
    }

    static std::vector<ElementWrapper> fromList(void *graalList) {
        using ListType = GraalList;
        using SizeType = decltype(ListType::size);

        if (!graalList) {
            return {};
        }

        std::vector<ElementWrapper> result{};

        auto list = static_cast<ListType *>(graalList);

        if (list->size > 0 && list->elements != nullptr) {
            for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
                if (list->elements[elementIndex]) {
                    result.emplace_back(ElementWrapper::fromGraal(static_cast<void *>(list->elements[elementIndex])));
                }
            }
        }

        return result;
    }
};

namespace math {
namespace detail {
// floorDiv() for y != 0.
static constexpr std::int64_t floorDivNonZero(std::int64_t x, std::int64_t y) noexcept {
    // INT64_MIN / -1 overflows (undefined behavior in C++); Java wraps the result to INT64_MIN.
    if (x == (std::numeric_limits<std::int64_t>::min)() && y == -1) {
        return x;
    }

    std::int64_t quotient = x / y;

    // The division truncates towards zero: when the signs differ and there is a remainder, round down.
    if (x % y != 0 && (x < 0) != (y < 0)) {
        quotient--;
    }

    return quotient;
}

// floorMod() for y != 0.
static constexpr std::int64_t floorModNonZero(std::int64_t x, std::int64_t y) noexcept {
    // x % -1 is 0, but INT64_MIN % -1 overflows (undefined behavior in C++).
    if (y == -1) {
        return 0;
    }

    std::int64_t remainder = x % y;

    // The remainder has the sign of x: when the signs differ, move it to the sign of y. |remainder| < |y| and the signs
    // are opposite, so the sum does not overflow.
    if (remainder != 0 && (remainder < 0) != (y < 0)) {
        remainder += y;
    }

    return remainder;
}
} // namespace detail

/**
 * Returns the largest value that is less than or equal to the algebraic quotient, as Java Math.floorDiv(long, long).
 * floorDiv(INT64_MIN, -1) is INT64_MIN, as in Java.
 *
 * @throws InvalidArgumentException if `y` is 0 (Java throws ArithmeticException).
 */
static constexpr std::int64_t floorDiv(std::int64_t x, std::int64_t y) {
    if (y == 0) {
        throw InvalidArgumentException("/ by zero");
    }

    return detail::floorDivNonZero(x, y);
}

/**
 * floorDiv(x, y) for a constant divisor `y`, not 0: does not throw.
 */
template <std::int64_t y>
    requires(y != 0)
static constexpr std::int64_t floorDiv(std::int64_t x) noexcept {
    return detail::floorDivNonZero(x, y);
}

/**
 * Returns the floor modulus `x - floorDiv(x, y) * y` (the sign of the result is the sign of `y`), as Java
 * Math.floorMod(long, long), without the overflow of that expression.
 *
 * @throws InvalidArgumentException if `y` is 0 (Java throws ArithmeticException).
 */
static constexpr std::int64_t floorMod(std::int64_t x, std::int64_t y) {
    if (y == 0) {
        throw InvalidArgumentException("/ by zero");
    }

    return detail::floorModNonZero(x, y);
}

/**
 * floorMod(x, y) for a constant divisor `y`, not 0: does not throw.
 */
template <std::int64_t y>
    requires(y != 0)
static constexpr std::int64_t floorMod(std::int64_t x) noexcept {
    return detail::floorModNonZero(x, y);
}

// The functions below compare floating-point numbers exactly on purpose.
DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wfloat-equal")
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wfloat-equal")

/**
 * Checks whether a double value can be safely converted to @c std::int64_t without losing its fractional part.
 *
 * @param value The value to check.
 * @return @c true if the value is finite, integral, and within the range of @c std::int64_t;
 *         @c false otherwise.
 */
inline bool isInt64(double value) noexcept {
    return std::isfinite(value) && value >= static_cast<double>((std::numeric_limits<std::int64_t>::min)()) &&
           value < -static_cast<double>((std::numeric_limits<std::int64_t>::min)()) && value == std::trunc(value);
}

/**
 * Checks whether two floating-point values are approximately equal using explicit relative and absolute tolerances.
 * Equal infinities compare as equal, while NaN values never compare as approximately equal.
 *
 * @tparam T The type of the first floating-point value.
 * @tparam U The type of the second floating-point value.
 * @param a The first value.
 * @param b The second value.
 * @param relativeTolerance The non-negative relative tolerance.
 * @param absoluteTolerance The non-negative absolute tolerance used for values close to zero.
 * @return @c true if the values are equal within either tolerance; @c false otherwise or if a tolerance is invalid.
 */
template <std::floating_point T, std::floating_point U>
inline bool approximatelyEquals(T a, U b, double relativeTolerance, double absoluteTolerance) noexcept {
    if (!std::isfinite(relativeTolerance) || !std::isfinite(absoluteTolerance) || relativeTolerance < 0.0 ||
        absoluteTolerance < 0.0) {
        return false;
    }

    using CommonType = std::common_type_t<T, U, double>;

    const auto first = static_cast<CommonType>(a);
    const auto second = static_cast<CommonType>(b);

    if (first == second) {
        return true;
    }

    if (!std::isfinite(first) || !std::isfinite(second)) {
        return false;
    }

    const auto difference = std::abs(first - second);
    const auto scale = (std::max)(std::abs(first), std::abs(second));

    return difference <=
           (std::max)(static_cast<CommonType>(absoluteTolerance), static_cast<CommonType>(relativeTolerance) * scale);
}

DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_GCC_WARNINGS_POP()

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
static constexpr std::int64_t getMillisFromNanos(std::int64_t timeNanos) noexcept {
    return math::floorDiv<NANOS_IN_MILLIS>(timeNanos);
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
static constexpr std::int32_t getNanoPartFromNanos(std::int64_t timeNanos) noexcept {
    return static_cast<std::int32_t>(math::floorMod<NANOS_IN_MILLIS>(timeNanos));
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
static constexpr std::int32_t getMillisFromTime(std::int64_t timeMillis) noexcept {
    return static_cast<std::int32_t>(math::floorMod<SECOND>(timeMillis));
}

/**
 * Returns correct number of seconds with proper handling negative values and overflows.
 * Idea is that number of milliseconds shall be within [0..999] interval
 *
 * @param timeMillis The timestamp in milliseconds
 * @return a correct number of second
 */
static constexpr std::int32_t getSecondsFromTime(std::int64_t timeMillis) noexcept {
    if (timeMillis >= 0) {
        return static_cast<std::int32_t>(
            (std::min)(timeMillis / SECOND, static_cast<std::int64_t>((std::numeric_limits<std::int32_t>::max)())));
    }

    return static_cast<std::int32_t>((std::max)((timeMillis + 1) / SECOND - 1,
                                                static_cast<std::int64_t>((std::numeric_limits<std::int32_t>::min)())));
}
} // namespace time_util

namespace math_util {

template <Integral T> constexpr static T abs(T a) {
    return a < 0 ? -a : a;
}

} // namespace math_util

namespace day_util {

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wunused-variable")
constexpr std::int32_t DAY_OF_YEAR[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
DXFCXX_DISABLE_GCC_WARNINGS_POP()

DXFCPP_EXPORT std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day);

} // namespace day_util

// The bit operations of the bits library: shifts with a defined result for any shift (0 or -1 for the type width or
// more, the other direction for a negative shift) and bitwise operations of mixed types. The names stay in dxfcpp.
using org::ttldtor::bits::Max;

using org::ttldtor::bits::leftArithmeticShift;
using org::ttldtor::bits::leftLogicalShift;
using org::ttldtor::bits::rightArithmeticShift;
using org::ttldtor::bits::rightLogicalShift;
using org::ttldtor::bits::sal;
using org::ttldtor::bits::sar;
using org::ttldtor::bits::shl;
using org::ttldtor::bits::shr;

using org::ttldtor::bits::andOp;
using org::ttldtor::bits::orOp;
using org::ttldtor::bits::xorOp;

template <Integral F, Integral M, Integral S> static constexpr F getBits(F flags, M mask, S shift) noexcept {
    return static_cast<F>(andOp(shr(flags, shift), mask));
}

template <Integral T> static constexpr T setBits(T flags, T mask, T shift, T bits) noexcept {
    if constexpr (std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;

        return static_cast<T>((static_cast<U>(flags) & ~(static_cast<U>(mask) << static_cast<U>(shift))) |
                              ((static_cast<U>(bits) & static_cast<U>(mask)) << static_cast<U>(shift)));
    } else {
        return static_cast<T>((flags & ~(mask << shift)) | ((bits & mask) << shift));
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

/**
 * Packs two values into one std::size_t, half of its bits each: `t` into the high half, `u` into the low one.
 *
 * @throws InvalidArgumentException if a value is negative or does not fit its half: it would change the other value.
 */
template <Integral T, Integral U> constexpr std::size_t pack(T t, U u) {
    constexpr auto sizeOfSize = sizeof(std::size_t) * CHAR_BIT;
    constexpr auto maxHalf = shr(~std::size_t{}, sizeOfSize / 2);

    if (std::cmp_less(t, 0) || std::cmp_greater(t, maxHalf) || std::cmp_less(u, 0) || std::cmp_greater(u, maxHalf)) {
        throw InvalidArgumentException("pack: a value does not fit half of std::size_t");
    }

    return orOp(shl(static_cast<std::size_t>(t), sizeOfSize / 2), static_cast<std::size_t>(u));
}

constexpr std::pair<std::size_t, std::size_t> unpack(std::size_t v) noexcept {
    constexpr auto sizeOfSize = sizeof(std::size_t) * CHAR_BIT;

    return {shr(v, sizeOfSize / 2), andOp(v, shr(~std::size_t{}, sizeOfSize / 2))};
}

/**
 * A simple wrapper around strings or something similar to strings to reduce the amount of code for methods that take
 * strings as input.
 */
struct StringLikeWrapperOld {
    using DataType = std::variant<std::string, std::string_view>;

    private:
    DataType data_{};

    public:
    StringLikeWrapperOld(std::string_view sv) : data_{sv} {
    }

    StringLikeWrapperOld(const char *chars) : data_{chars == nullptr ? std::string_view{} : std::string_view{chars}} {
    }

    StringLikeWrapperOld(const std::string &s) : data_{s} {
    }

    StringLikeWrapperOld(std::string &&s) : data_{std::move(s)} {
    }

    template <auto N>
    StringLikeWrapperOld(const char (&chars)[N]) : StringLikeWrapperOld{std::string_view{chars, chars + N}} {
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

    bool ends_with(const StringLikeWrapperOld &sw) const {
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

    bool operator==(const StringLikeWrapperOld &sw) const {
        return sw.operator std::string_view() == this->operator std::string_view();
    }

    friend std::string operator+(const StringLikeWrapperOld &sw1, const StringLikeWrapperOld &sw2) {
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

namespace util {

void throwInvalidChar(char c, const std::string &name);

inline void checkChar(char c, std::uint32_t mask, const std::string &name) {
    if ((andOp(c, ~mask)) != 0) {
        throwInvalidChar(c, name);
    }
}

} // namespace util

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
