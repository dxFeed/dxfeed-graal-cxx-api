// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string_view>

#include "../../internal/TimeFormat.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedTimeFormat {

/* dxfg_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat> getDefault();
/* dxfg_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat> getGmt();
/* dxfg_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat>
withTimeZone(/* dxfg_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat);
/* dxfg_time_format_t* */ JavaObjectHandle<dxfcpp::TimeFormat>
withMillis(/* dxfg_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat);
std::int64_t parse(/* dxfg_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat,
                   std::string_view value);
std::string format(/* dxfg_time_format_t* */ const JavaObjectHandle<dxfcpp::TimeFormat> &timeFormat,
                   std::int64_t value);

} // namespace IsolatedTimeFormat

} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()