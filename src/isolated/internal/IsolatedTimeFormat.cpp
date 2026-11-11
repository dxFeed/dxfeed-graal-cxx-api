// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedTimeFormat.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedTimeFormat {

/* dxfg_v4_time_format_t* */ JavaObjectHandle<TimeFormat> getDefault() {
    return JavaObjectHandle<TimeFormat>(runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeFormat_DEFAULT));
}

/* dxfg_v4_time_format_t* */ JavaObjectHandle<TimeFormat> getGmt() {
    return JavaObjectHandle<TimeFormat>(runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeFormat_GMT));
}

/* dxfg_v4_time_format_t* */ JavaObjectHandle<TimeFormat>
withTimeZone(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimeFormat_withTimeZone`. The `timeFormat` handle is invalid");
    }

    return JavaObjectHandle<TimeFormat>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeFormat_withTimeZone, static_cast<dxfg_v4_time_format_t *>(timeFormat.get())));
}

/* dxfg_v4_time_format_t* */ JavaObjectHandle<TimeFormat>
withMillis(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimeFormat_withMillis`. The `timeFormat` handle is invalid");
    }

    return JavaObjectHandle<TimeFormat>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeFormat_withMillis, static_cast<dxfg_v4_time_format_t *>(timeFormat.get())));
}

std::int64_t parse(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat,
                   std::string_view value) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimeFormat_parse`. The `timeFormat` handle is invalid");
    }

    return runGraalV4FunctionWithResult(
        dxfg_v4_TimeFormat_parse, static_cast<dxfg_v4_time_format_t *>(timeFormat.get()),
        SdkString{value, "TimeFormat::parse(value)"}.c_str());
}

std::string format(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<TimeFormat> &timeFormat,
                   std::int64_t value) {
    if (!timeFormat) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TimeFormat_format`. The `timeFormat` handle is invalid");
    }

    const auto format = IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeFormat_format, static_cast<dxfg_v4_time_format_t *>(timeFormat.get()), value));
    return dxfcpp::toString(format.get());
}

} // namespace IsolatedTimeFormat

} // namespace isolated::internal

DXFCPP_END_NAMESPACE
