// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/JavaException.hpp"
#include "../internal/Isolate.hpp"

struct __graal_isolatethread_t;
typedef struct __graal_isolatethread_t graal_isolatethread_t;

DXFCPP_BEGIN_NAMESPACE

namespace isolated {

inline std::string_view toString(dxfg_event_clazz_t eventClazz) {
    switch (eventClazz) {
    case DXFG_EVENT_QUOTE:
        return "DXFG_EVENT_QUOTE";
    case DXFG_EVENT_PROFILE:
        return "DXFG_EVENT_PROFILE";
    case DXFG_EVENT_SUMMARY:
        return "DXFG_EVENT_SUMMARY";
    case DXFG_EVENT_GREEKS:
        return "DXFG_EVENT_GREEKS";
    case DXFG_EVENT_CANDLE:
        return "DXFG_EVENT_CANDLE";
    case DXFG_EVENT_DAILY_CANDLE:
        return "DXFG_EVENT_DAILY_CANDLE";
    case DXFG_EVENT_UNDERLYING:
        return "DXFG_EVENT_UNDERLYING";
    case DXFG_EVENT_THEO_PRICE:
        return "DXFG_EVENT_THEO_PRICE";
    case DXFG_EVENT_TRADE:
        return "DXFG_EVENT_TRADE";
    case DXFG_EVENT_TRADE_ETH:
        return "DXFG_EVENT_TRADE_ETH";
    case DXFG_EVENT_CONFIGURATION:
        return "DXFG_EVENT_CONFIGURATION";
    case DXFG_EVENT_MESSAGE:
        return "DXFG_EVENT_MESSAGE";
    case DXFG_EVENT_TIME_AND_SALE:
        return "DXFG_EVENT_TIME_AND_SALE";
    case DXFG_EVENT_ORDER_BASE:
        return "DXFG_EVENT_ORDER_BASE";
    case DXFG_EVENT_ORDER:
        return "DXFG_EVENT_ORDER";
    case DXFG_EVENT_ANALYTIC_ORDER:
        return "DXFG_EVENT_ANALYTIC_ORDER";
    case DXFG_EVENT_OTC_MARKETS_ORDER:
        return "DXFG_EVENT_OTC_MARKETS_ORDER";
    case DXFG_EVENT_SPREAD_ORDER:
        return "DXFG_EVENT_SPREAD_ORDER";
    case DXFG_EVENT_SERIES:
        return "DXFG_EVENT_SERIES";
    case DXFG_EVENT_OPTION_SALE:
        return "DXFG_EVENT_OPTION_SALE";
    case DXFG_EVENT_TEXT_MESSAGE:
        return "DXFG_EVENT_TEXT_MESSAGE";
    case DXFG_EVENT_MARKET_MAKER:
        return "DXFG_EVENT_MARKET_MAKER";
    case DXFG_EVENT_TEXT_CONFIGURATION:
        return "DXFG_EVENT_TEXT_CONFIGURATION";
    default:
        return "UNKNOWN";
    }
}

constexpr auto throwIfNullptr = [](auto v) {
    return JavaException::throwIfNullptr(v);
};

constexpr auto throwIfLessThanZero = [](auto v) {
    return JavaException::throwIfLessThanZero(v);
};

constexpr auto throwIfMinusOne = [](auto v) {
    return JavaException::throwIfMinusOne(v);
};

constexpr auto throwIfMinusMin = [](auto v) {
    return JavaException::throwIfMinusMin(v);
};

constexpr auto throwIfMinusInf = [](auto v) {
    return JavaException::throwIfMinusInf(v);
};

constexpr auto runGraalFunction(auto graalFunction, auto &&...params) {
    return runIsolatedThrow(
        [](auto threadHandle, auto &&graalFunction, auto &&...params) {
            return graalFunction(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrow(auto resultCheckerConverter, auto graalFunction, auto &&...params) {
    return runIsolatedThrow(
        [](auto threadHandle, auto &&resultCheckerConverter, auto &&graalFunction, auto &&...params) {
            return resultCheckerConverter(graalFunction(static_cast<graal_isolatethread_t *>(threadHandle), params...));
        },
        resultCheckerConverter, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfNullptr(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfNullptr, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfLessThanZero(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfLessThanZero, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfMinusOne(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfMinusOne, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfMinusMin(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfMinusMin, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfMinusInf(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfMinusInf, graalFunction, params...);
}

} // namespace isolated

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()