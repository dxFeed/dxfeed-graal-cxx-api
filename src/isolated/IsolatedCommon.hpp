// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"
#include "../internal/Isolate.hpp"
#include "SdkString.hpp"

#include <dxfg_api.h>

#include <memory>
#include <optional>

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
    case DXFG_EVENT_ORDER_IMBALANCE:
        return "DXFG_EVENT_ORDER_IMBALANCE";
    case DXFG_EVENT_NUAM_ORDER:
        return "DXFG_EVENT_NUAM_ORDER";
    case DXFG_EVENT_NUAM_TRADE:
        return "DXFG_EVENT_NUAM_TRADE";
    case DXFG_EVENT_NUAM_TIME_AND_SALE:
        return "DXFG_EVENT_NUAM_TIME_AND_SALE";
    default:
        return "UNKNOWN";
    }
}

/// Releases a Java exception taken from the thread (`dxfg_v4_Exception_release`, the whole chain of causes).
struct ThreadExceptionDeleter {
    graal_isolatethread_t *thread;

    void operator()(dxfg_v4_exception_t *exception) const noexcept;
};

/// The Java exception taken from the thread; it is released on the isolate thread that took it.
using ThreadException = std::unique_ptr<dxfg_v4_exception_t, ThreadExceptionDeleter>;

/**
 * Takes and clears the Java exception that a failed SDK function left in the calling thread
 * (`dxfg_v4_get_and_clear_thread_exception`). Call it on the thread of the failed call, right after the failure: a
 * successful call does not clear the exception.
 *
 * @param thread The isolate thread of the current thread.
 * @return The exception, an empty pointer if the thread has none, or `std::nullopt` if the SDK failed to take it. The
 * thread is left without an exception in every case.
 */
std::optional<ThreadException> fetchThreadException(graal_isolatethread_t *thread) noexcept;

/**
 * Throws the Java exception that a failed (`DXFG_EXECUTE_FAIL`) SDK function left in the calling thread. The thread is
 * left without an exception.
 *
 * @param thread The isolate thread of the current thread.
 * @throws JavaException The Java exception of the thread.
 * @throws RuntimeException The thread has no Java exception, or the SDK failed to take it.
 */
[[noreturn]] void throwThreadException(graal_isolatethread_t *thread);

/**
 * Throws RuntimeException for a null result of an SDK function that does not return `NULL` as a normal result.
 */
[[noreturn]] void throwUnexpectedNullResult();

/**
 * Reads and clears the Java exception that a failed SDK function left in the thread. It is for a direct call that turns
 * a failure into a result instead of an exception: the SDK keeps the exception until it is read, so a later, unrelated
 * call would get it otherwise.
 *
 * @param thread The isolate thread of the current thread.
 */
void clearThreadException(graal_isolatethread_t *thread) noexcept;

/**
 * Sets a system property of the SDK (`dxfg_system_set_property`). A failure is a result: the Java exception of the
 * failed call is cleared.
 *
 * @param thread The isolate thread of the current thread.
 * @param key The name of the property.
 * @param value The value of the property.
 * @return `true` if the property is set.
 */
bool setSystemProperty(graal_isolatethread_t *thread, const char *key, const char *value) noexcept;

constexpr auto throwIfNullptr = [](auto v) {
    return JavaException::throwIfNullptr(v);
};

constexpr auto nullableOrThrow = [](auto v) {
    return JavaException::nullableOrThrow(v);
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

constexpr auto runGraalFunctionAndThrow(auto resultCheckerConverter, auto graalFunction, auto &&...params) {
    return runIsolatedThrow(
        [](auto threadHandle, auto &&checkerConverter, auto &&function, auto &&...functionParams) {
            return checkerConverter(function(static_cast<graal_isolatethread_t *>(threadHandle), functionParams...));
        },
        resultCheckerConverter, graalFunction, params...);
}

constexpr auto runGraalFunctionAndThrowIfNullptr(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(throwIfNullptr, graalFunction, params...);
}

constexpr auto runGraalFunctionNullableOrThrow(auto graalFunction, auto &&...params) {
    return runGraalFunctionAndThrow(nullableOrThrow, graalFunction, params...);
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