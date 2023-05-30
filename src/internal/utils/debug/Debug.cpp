// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <chrono>
#include <sstream>
#include <thread>

namespace dxfcpp {

#if DXFCPP_DEBUG == 0
#else
template <typename... Args> std::string vformat(std::string_view format, Args &&...args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}

template <typename... Args> void vprint(std::ostream &os, std::string_view format, Args &&...args) {
    fmt::vprint(os, format, fmt::make_format_args(args...));
}

std::string Debugger::nowStr() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d %H%M%S}.{:0>3}", std::chrono::floor<std::chrono::seconds>(now), ms);
}

std::string Debugger::nowStrWithTimeZone() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", std::chrono::floor<std::chrono::seconds>(now), ms,
                       std::chrono::floor<std::chrono::seconds>(now));
}

std::string Debugger::debugPrefixStr() {
    return fmt::format("D {} [{}]", nowStr(), toString(std::this_thread::get_id()));
}

 void Debugger::debug(std::string str) {
     vprint(std::cerr, "{} {}\n", debugPrefixStr(), std::move(str));
 }

#    if DXFCPP_TRACE == 1
std::string Debugger::tracePrefixStr() {
    return fmt::format("T {} [{}]", nowStr(), toString(std::this_thread::get_id()));
}

void Debugger::trace(std::string str) {
    vprint(std::cerr, "{} ~~ {}\n", tracePrefixStr(), std::move(str));
}
#    else
inline void Debugger::trace(...) {}
#    endif

#endif

} // namespace dxfcpp
