// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/util/IsolatedTimePeriod.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::util::IsolatedTimePeriod {
/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> ZERO() {
    return JavaObjectHandle<TimePeriod>(runGraalFunctionAndThrowIfNullptr(dxfg_TimePeriod_ZERO));
}

/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> UNLIMITED() {
    return JavaObjectHandle<TimePeriod>(runGraalFunctionAndThrowIfNullptr(dxfg_TimePeriod_UNLIMITED));
}

/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::int64_t value) {
    return JavaObjectHandle<TimePeriod>(runGraalFunctionAndThrowIfNullptr(dxfg_TimePeriod_valueOf, value));
}

/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(const StringLike& value) {
    return JavaObjectHandle<TimePeriod>(runGraalFunctionAndThrowIfNullptr(dxfg_TimePeriod_valueOf2, value.data()));
}

std::int64_t getTime(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimePeriod_getTime`. The `timePeriod` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimePeriod_getTime,
                                              static_cast<dxfg_time_period_t *>(timePeriod.get()));
}

std::int32_t getSeconds(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimePeriod_getSeconds`. The `timePeriod` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimePeriod_getSeconds,
                                              static_cast<dxfg_time_period_t *>(timePeriod.get()));
}

std::int64_t getNanos(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimePeriod_getNanos`. The `timePeriod` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimePeriod_getNanos,
                                              static_cast<dxfg_time_period_t *>(timePeriod.get()));
}

} // namespace isolated::util::IsolatedTimePeriod

DXFCPP_END_NAMESPACE
