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
template <typename... Args> static std::string vformat(std::string_view format, Args &&...args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}

template <typename... Args> void static vprint(std::ostream &os, std::string_view format, Args &&...args) {
    fmt::vprint(os, format, fmt::make_format_args(args...));
}

static inline std::string nowStr() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d %H%M%S}.{:0>3}", std::chrono::floor<std::chrono::seconds>(now), ms);
}

static inline std::string nowStrWithTimeZone() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    return fmt::format("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", std::chrono::floor<std::chrono::seconds>(now), ms,
                       std::chrono::floor<std::chrono::seconds>(now));
}

static inline std::string debugPrefixStr() {
    return fmt::format("D {} [{}]", nowStr(), toString(std::this_thread::get_id()));
}

static inline std::string tracePrefixStr() {
    return fmt::format("T {} [{}]", nowStr(), toString(std::this_thread::get_id()));
}

template <typename... Args> void Debugger::debug(std::string_view format, Args &&...args) {
    vprint(std::cerr, "{} {}\n", debugPrefixStr(), vformat(format, std::forward<Args>(args)...));
}

#    if DXFCPP_TRACE == 1
template <typename... Args> void Debugger::trace(std::string_view format, Args &&...args) {
    vprint(std::cerr, "{} ~~ {}\n", tracePrefixStr(), vformat(format, std::forward<Args>(args)...));
}
#    else
inline void Debugger::trace(...) {}
#    endif

#endif

} // namespace dxfcpp

#if DXFCPP_DEBUG == 0
#else
template void dxfcpp::Debugger::debug(std::string_view);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&);
template void dxfcpp::Debugger::debug(std::string_view, const std::string &);
template void dxfcpp::Debugger::debug(std::string_view, const char *&&);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, const std::string &);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, std::string &&);
template void dxfcpp::Debugger::debug(std::string_view, const std::string &, const std::string &);
template void dxfcpp::Debugger::debug(std::string_view, const std::string &, std::string &);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const (&)[1]);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const (&)[2]);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const (&)[3]);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const (&)[4]);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const (&)[5]);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, std::string_view &);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, std::string &);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, std::size_t &&);
template void dxfcpp::Debugger::debug(std::string_view, void *&);
template void dxfcpp::Debugger::debug(std::string_view, void *&, char const *&&, std::size_t &&);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, char const *&&);
template void dxfcpp::Debugger::debug(std::string_view, std::string &&, std::string const &, std::string const &);
template void dxfcpp::Debugger::debug(std::string_view, std::string const &, std::string const &, bool &);

#    if DXFCPP_TRACE == 1
template void dxfcpp::Debugger::trace(std::string_view);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&);
template void dxfcpp::Debugger::trace(std::string_view, const std::string &);
template void dxfcpp::Debugger::trace(std::string_view, const char *&&);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, const std::string &);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, std::string &&);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, std::size_t &);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, void *&&);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, std::size_t &&);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, bool &&);
template void dxfcpp::Debugger::trace(std::string_view, std::string &&, std::size_t &, unsigned int &);
#    endif
#endif