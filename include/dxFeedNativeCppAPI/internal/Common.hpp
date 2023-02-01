// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif
#include <cstring>

#include <chrono>
#include <type_traits>
#include <iostream>
#include <sstream>

#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/chrono.h>

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

inline auto now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

template <typename... Args>
std::string vformat(std::string_view format, Args&&... args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}

template <typename... Args>
void vprint(std::ostream& os, std::string_view format, Args&&... args) {
    fmt::vprint(os, format, fmt::make_format_args(args...));
}

inline std::string nowStr() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return vformat("{:%d%m%y %H%M%S}.{:0>3}", std::chrono::floor<std::chrono::seconds>(now), ms);
}

inline std::string debugPrefixStr() {
    std::ostringstream tid{};

    tid << std::this_thread::get_id();

    return vformat("D {} [{}]", nowStr(), tid.str());
}

template <typename... Args>
inline void debug(std::string_view format, Args&&... args) {
    vprint(std::clog, "{} {}\n", debugPrefixStr(), vformat(format, std::forward<Args>(args)...));
}


} // namespace dxfcpp::detail