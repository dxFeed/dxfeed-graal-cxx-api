// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif
#include <cstring>

#include <chrono>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <mutex>

#include <dxfg_javac.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/std.h>

namespace dxfcpp::detail {
#ifdef NDEBUG
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

inline std::string debugPrefixStr() {
    std::ostringstream tid{};

    tid << std::this_thread::get_id();

    return vformat("D {} [{}]", nowStr(), tid.str());
}

template <typename... Args> inline void debug(std::string_view format, Args &&...args) {
    vprint(std::cerr, "{} {}\n", debugPrefixStr(), vformat(format, std::forward<Args>(args)...));
}

/**
 * Non thread-safe
 *
 * @tparam HandleType The handle type (must be pointer)
 */
template <typename HandleType>
    requires requires { std::is_pointer_v<HandleType>; }
struct WithHandle {
  protected:
    HandleType handle_ = nullptr;

    void releaseHandle() {
        if constexpr (detail::isDebug) {
            detail::debug("WithHandle::releaseHandle(handle_ = {})", bit_cast<std::size_t>(handle_));
        }

        if (!handle_) {
            return;
        }

        runIsolatedOrElse(
            [this](auto threadHandle) {
                if (handle_) {
                    return dxfg_JavaObjectHandler_release(threadHandle,
                                                          detail::bit_cast<dxfg_java_object_handler *>(handle_)) == 0;
                }

                return true;
            },
            false);

        handle_ = nullptr;
    }
};

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += fmt::format("'{}'{}", it->getName(), std::next(it) == end ? "" : ", ");
    }

    return result + "]";
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

} // namespace dxfcpp::detail