// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedTimeFormat.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedTimeFormat {

/* dxfg_time_format_t* */ JavaObjectHandle<TimeFormat> getDefault() {
    return JavaObjectHandle<TimeFormat>(runGraalFunctionAndThrowIfNullptr(dxfg_TimeFormat_DEFAULT));
}

/* dxfg_time_format_t* */ JavaObjectHandle<TimeFormat> getGmt() {
    return JavaObjectHandle<TimeFormat>(runGraalFunctionAndThrowIfNullptr(dxfg_TimeFormat_GMT));
}

/* dxfg_time_format_t* */ JavaObjectHandle<TimeFormat>
withTimeZone(/* dxfg_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeFormat_withTimeZone`. The `timeFormat` handle is invalid");
    }

    return JavaObjectHandle<TimeFormat>(runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeFormat_withTimeZone, static_cast<dxfg_time_format_t *>(timeFormat.get())));
}

/* dxfg_time_format_t* */ JavaObjectHandle<TimeFormat>
withMillis(/* dxfg_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeFormat_withMillis`. The `timeFormat` handle is invalid");
    }

    return JavaObjectHandle<TimeFormat>(runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeFormat_withMillis, static_cast<dxfg_time_format_t *>(timeFormat.get())));
}

std::int64_t parse(/* dxfg_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat,
                   const StringLike& value) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeFormat_parse`. The `timeFormat` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimeFormat_parse,
                                              static_cast<dxfg_time_format_t *>(timeFormat.get()), value.data());
}

std::string format(/* dxfg_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat,
                   std::int64_t value) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeFormat_format`. The `timeFormat` handle is invalid");
    }

    auto format = runGraalFunctionAndThrowIfNullptr(dxfg_TimeFormat_format,
                                                    static_cast<dxfg_time_format_t *>(timeFormat.get()), value);
    auto result = dxfcpp::toString(format);

    IsolatedString::release(format);

    return result;
}

} // namespace IsolatedTimeFormat

} // namespace isolated::internal

DXFCPP_END_NAMESPACE