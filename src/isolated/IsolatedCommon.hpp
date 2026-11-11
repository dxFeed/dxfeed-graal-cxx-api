// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"
#include "../internal/InternalExport.hpp"
#include "../internal/Isolate.hpp"
#include "SdkString.hpp"

#include <dxfg_api.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>

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
struct DXFCPP_INTERNAL_EXPORT ThreadExceptionDeleter {
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
DXFCPP_INTERNAL_EXPORT std::optional<ThreadException> fetchThreadException(graal_isolatethread_t *thread) noexcept;

/**
 * Throws the Java exception that a failed (`DXFG_EXECUTE_FAIL`) SDK function left in the calling thread. The thread is
 * left without an exception.
 *
 * @param thread The isolate thread of the current thread.
 * @throws JavaException The Java exception of the thread.
 * @throws RuntimeException The thread has no Java exception, or the SDK failed to take it.
 */
[[noreturn]] DXFCPP_INTERNAL_EXPORT void throwThreadException(graal_isolatethread_t *thread);

/**
 * Checks the status of a dxfg_v4_ function: a status other than `DXFG_EXECUTE_SUCCESSFULLY` throws the Java exception
 * of the calling thread (throwThreadException()).
 *
 * @param thread The isolate thread of the current thread.
 * @param status The status that the function returned.
 */
inline void throwIfFailed(graal_isolatethread_t *thread, std::int32_t status) {
    if (status != DXFG_EXECUTE_SUCCESSFULLY) {
        throwThreadException(thread);
    }
}

/**
 * Throws RuntimeException for a null result of an SDK function that does not return `NULL` as a normal result.
 */
[[noreturn]] DXFCPP_INTERNAL_EXPORT void throwUnexpectedNullResult();

/**
 * Reads and clears the Java exception that a failed SDK function left in the thread. It is for a direct call that turns
 * a failure into a result instead of an exception: the SDK keeps the exception until it is read, so a later, unrelated
 * call would get it otherwise.
 *
 * @param thread The isolate thread of the current thread.
 */
void clearThreadException(graal_isolatethread_t *thread) noexcept;

/**
 * Sets a system property of the SDK (`dxfg_v4_system_set_property`). A failure is a result: the Java exception of the
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

// The calls of the dxfg_v4_ functions: each returns DXFG_EXECUTE_SUCCESSFULLY or DXFG_EXECUTE_FAIL and writes its result
// to the last (out) parameter. Any other status is a failure here too: the functions with positive statuses (await of
// a promise, the event queue) need a call of their own.

/// The type of the result that a dxfg_v4_ function writes to its last parameter.
template <typename Function> struct GraalV4Result;

template <typename... Params> struct GraalV4Result<std::int32_t (*)(graal_isolatethread_t *, Params...)> {
    static_assert(sizeof...(Params) > 0, "the function has no out parameter");

    using Type = std::remove_pointer_t<std::tuple_element_t<sizeof...(Params) - 1, std::tuple<Params...>>>;
};

/**
 * Calls a dxfg_v4_ function without a result: `graalFunction(thread, params...)`.
 *
 * @throws JavaException The function failed with a Java exception.
 * @throws RuntimeException The function failed without a Java exception.
 */
inline void runGraalV4Function(auto graalFunction, auto &&...params) {
    runIsolatedThrow(
        [](auto threadHandle, auto &&function, auto &&...functionParams) {
            auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);

            throwIfFailed(thread, function(thread, functionParams...));
        },
        graalFunction, params...);
}

/**
 * Calls a dxfg_v4_ function with a result: `graalFunction(thread, params..., &result)`.
 *
 * @return The result; `NULL` is a valid result.
 * @throws JavaException The function failed with a Java exception.
 * @throws RuntimeException The function failed without a Java exception.
 */
inline auto runGraalV4FunctionWithResult(auto graalFunction, auto &&...params) {
    return runIsolatedThrow(
        [](auto threadHandle, auto &&function, auto &&...functionParams) {
            auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);
            typename GraalV4Result<std::decay_t<decltype(function)>>::Type result{};

            throwIfFailed(thread, function(thread, functionParams..., &result));

            return result;
        },
        graalFunction, params...);
}

/**
 * Calls a dxfg_v4_ function whose result is not `NULL` on success (a function that creates an object, for example):
 * `graalFunction(thread, params..., &result)`.
 *
 * @return The result.
 * @throws JavaException The function failed with a Java exception.
 * @throws RuntimeException The function failed without a Java exception, or the result is `NULL`.
 */
inline auto runGraalV4FunctionWithNonNullResult(auto graalFunction, auto &&...params) {
    auto result = runGraalV4FunctionWithResult(graalFunction, params...);

    if (result == nullptr) {
        throwUnexpectedNullResult();
    }

    return result;
}

} // namespace isolated

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()