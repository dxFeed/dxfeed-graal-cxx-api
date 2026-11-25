// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>

#include "../../../include/dxfeed_graal_cpp_api/util/TimeFormat.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedTimeFormat {

/// dxfg_v4_TimeFormat_DEFAULT
/* dxfg_v4_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat> getDefault();
/// dxfg_v4_TimeFormat_GMT
/* dxfg_v4_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat> getGmt();
/// dxfg_v4_TimeFormat_withTimeZone
/* dxfg_v4_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat>
withTimeZone(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat);
/// dxfg_v4_TimeFormat_withMillis
/* dxfg_v4_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat>
withMillis(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat);
/// dxfg_v4_TimeFormat_parse
std::int64_t parse(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat,
                   std::string_view value);
/// dxfg_v4_TimeFormat_format
std::string format(/* dxfg_v4_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat,
                   std::int64_t value);

} // namespace IsolatedTimeFormat

} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
