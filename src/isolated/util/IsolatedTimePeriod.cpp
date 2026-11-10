// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedTimePeriod.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::util::IsolatedTimePeriod {
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> ZERO() {
    return JavaObjectHandle<TimePeriod>(runGraalV4FunctionWithNonNullResult(dxfg_v4_TimePeriod_ZERO));
}

/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> UNLIMITED() {
    return JavaObjectHandle<TimePeriod>(runGraalV4FunctionWithNonNullResult(dxfg_v4_TimePeriod_UNLIMITED));
}

/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::int64_t value) {
    return JavaObjectHandle<TimePeriod>(runGraalV4FunctionWithNonNullResult(dxfg_v4_TimePeriod_valueOf, value));
}

/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::string_view value) {
    return JavaObjectHandle<TimePeriod>(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_TimePeriod_valueOfString, SdkString{value}.c_str()));
}

std::int64_t getTime(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimePeriod_getTime`. The `timePeriod` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_TimePeriod_getTime,
                                        static_cast<dxfg_v4_time_period_t *>(timePeriod.get()));
}

std::int32_t getSeconds(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimePeriod_getSeconds`. The `timePeriod` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_TimePeriod_getSeconds,
                                        static_cast<dxfg_v4_time_period_t *>(timePeriod.get()));
}

std::int64_t getNanos(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod) {
    if (!timePeriod) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimePeriod_getNanos`. The `timePeriod` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_TimePeriod_getNanos,
                                        static_cast<dxfg_v4_time_period_t *>(timePeriod.get()));
}

} // namespace isolated::util::IsolatedTimePeriod

DXFCPP_END_NAMESPACE
