// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/JavaException.hpp"
#include "../internal/Isolate.hpp"

#include "../api/DXEndpoint.hpp"

struct __graal_isolatethread_t;
typedef struct __graal_isolatethread_t graal_isolatethread_t;

DXFCPP_BEGIN_NAMESPACE

namespace isolated {

inline constexpr dxfcpp::DXEndpoint::State graalStateToState(dxfg_endpoint_state_t state) noexcept {
    switch (state) {
    case DXFG_ENDPOINT_STATE_NOT_CONNECTED:
        return dxfcpp::DXEndpoint::State::NOT_CONNECTED;
    case DXFG_ENDPOINT_STATE_CONNECTING:
        return dxfcpp::DXEndpoint::State::CONNECTING;
    case DXFG_ENDPOINT_STATE_CONNECTED:
        return dxfcpp::DXEndpoint::State::CONNECTED;
    case DXFG_ENDPOINT_STATE_CLOSED:
        return dxfcpp::DXEndpoint::State::CLOSED;
    }

    return dxfcpp::DXEndpoint::State::NOT_CONNECTED;
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