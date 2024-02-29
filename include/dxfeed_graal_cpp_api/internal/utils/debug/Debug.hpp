// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

// #define DXFCPP_DEBUG 1
// #define DXFCPP_TRACE_LISTS 1
// #define DXFCPP_TRACE_ISOLATES 1

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

DXFCPP_BEGIN_NAMESPACE

#if DXFCPP_DEBUG == 0

static inline std::string getDebugName() {
    return {};
}

struct DXFCPP_EXPORT Debugger {
    static constexpr bool isDebug = false;
    static constexpr bool traceIsolates = false;
    static constexpr bool traceLists = false;

    static void debug(std::string) {
    }
    static void trace(std::string) {
    }
};

#else

struct DXFCPP_EXPORT Debugger {
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
    static std::string nowStr();
    static std::string nowStrWithTimeZone();
    static std::string debugPrefixStr();
    static void debug(std::string);
#    if DXFCPP_TRACE == 1
    static std::string tracePrefixStr();
    static void trace(std::string);
#    else
    static void trace(std::string) {
    }
#    endif
};

#endif

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
