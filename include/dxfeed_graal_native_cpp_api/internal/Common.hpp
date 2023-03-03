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
#include <fmt/std.h>

namespace dxfcpp::detail {
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
template <typename To, typename From, std::enable_if_t<sizeof(To) == sizeof(From), int> = 0>
constexpr auto bit_cast(const From &from) -> To {
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

    return vformat("{:%y%m%d %H%M%S}.{:0>3}", std::chrono::floor<std::chrono::seconds>(now), ms);
}

inline std::string nowStrWithTimeZone() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return vformat("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", std::chrono::floor<std::chrono::seconds>(now), ms,
                   std::chrono::floor<std::chrono::seconds>(now));
}

inline std::string formatTimeStamp(std::int64_t timestamp) {
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return vformat("{:%y%m%d-%H%M%S%z}", tm);
}

inline std::string formatTimeStampWithMillis(std::int64_t timestamp) {
    auto ms = timestamp % 1000;
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return vformat("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", tm, ms, tm);
}

inline std::string debugPrefixStr() {
    std::ostringstream tid{};

    tid << std::this_thread::get_id();

    return vformat("D {} [{}]", nowStr(), tid.str());
}

template <typename... Args> inline void debug(std::string_view format, Args &&...args) {
    vprint(std::cerr, "{} {}\n", debugPrefixStr(), vformat(format, std::forward<Args>(args)...));
}

namespace handler_utils {
void javaObjectHandlerDeleter(void *javaObjectHandler);

using JavaObjectHandler = std::unique_ptr<void, decltype(&javaObjectHandlerDeleter)>;

inline JavaObjectHandler createJavaObjectHandler(void *handler = nullptr);

inline std::string toString(const JavaObjectHandler &handler);

struct EventClassList {
    static std::unique_ptr<EventClassList> create(std::size_t size) noexcept;

    void set(std::size_t index, std::uint32_t id) noexcept;

    bool isEmpty() const noexcept;

    std::size_t size() const noexcept;

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
static const std::int64_t NANOS_IN_MILLIS = 1'000'000LL;

static std::int64_t getNanosFromMillisAndNanoPart(std::int64_t timeMillis, std::int32_t timeNanoPart) {
    return (timeMillis * NANOS_IN_MILLIS) + timeNanoPart;
}
} // namespace time_nanos_util

namespace time_util {
/// Number of milliseconds in a second.
static const std::int64_t SECOND = 1000LL;

/**
 * Returns correct number of milliseconds with proper handling negative values.
 * Idea is that number of milliseconds shall be within [0..999] interval
 *
 * @param timeMillis
 * @return
 */
static std::int32_t getMillisFromTime(std::int64_t timeMillis) {
    return static_cast<std::int32_t>(math::floorMod(timeMillis, SECOND));
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

    return std::string("\\u") + vformat("{0:04x}", 65536 + static_cast<int>(c)).substr(1);
}

} // namespace string_util

namespace util {

template <typename T>
concept Integral = std::is_integral<T>::value;

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

template <typename... Ts> using Max = typename detail::MaxImpl<Ts...>::Type;

template <Integral T> static constexpr T getBits(T flags, T mask, T shift) {
    if constexpr (std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;

        return static_cast<T>((static_cast<U>(flags) >> static_cast<U>(shift)) & static_cast<U>(mask));
    } else {
        return (flags >> shift) & mask;
    }
}

template <Integral F, Integral M, Integral S> static constexpr F getBits(F flags, M mask, S shift) {
    if constexpr (std::is_signed_v<F> || std::is_signed_v<M> || std::is_signed_v<S>) {
        using U = std::make_unsigned_t<Max<F, M, S>>;

        return static_cast<F>((static_cast<U>(flags) >> static_cast<U>(shift)) & static_cast<U>(mask));
    } else {
        return (flags >> shift) & mask;
    }
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

} // namespace util

} // namespace dxfcpp::detail