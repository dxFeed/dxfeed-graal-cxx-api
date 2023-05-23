// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#define DXFCPP_DEBUG 1
#define DXFCPP_TRACE_LISTS 1

#ifndef DXFCPP_DEBUG
#    define DXFCPP_DEBUG 0
#endif

#ifndef DXFCPP_TRACE_ISOLATES
#    define DXFCPP_TRACE_ISOLATES 0
#endif
#ifndef DXFCPP_TRACE_LISTS
#    define DXFCPP_TRACE_LISTS 0
#endif

#if DXFCPP_TRACE_ISOLATES == 1 || DXFCPP_TRACE_LISTS == 1
#    define DXFCPP_TRACE DXFCPP_DEBUG
#endif

namespace dxfcpp {

#if DXFCPP_DEBUG == 0

struct Debugger {
    static constexpr bool isDebug = false;
    static constexpr bool traceIsolates = false;
    static constexpr bool traceLists = false;

    static void debug(...) {}
    static void trace(...) {}
};

#else

struct Debugger {
    static constexpr bool isDebug = true;
#    if DXFCPP_TRACE_ISOLATES == 1
    static constexpr bool traceIsolates = true;
#    else
    static constexpr bool traceIsolates = false;
#    endif

#    if DXFCPP_TRACE_LISTS == 1
    static constexpr bool traceLists = true;
#    else
    static constexpr bool traceLists = false;
#    endif
    template <typename... Args> static void debug(std::string_view format, Args &&...args);
#    if DXFCPP_TRACE == 1
    template <typename... Args> static void trace(std::string_view format, Args &&...args);
#    else
    static void trace(...);
#    endif
};

#endif

} // namespace dxfcpp
