// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/util/TimePeriod.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::util::IsolatedTimePeriod {

/// dxfg_v4_TimePeriod_ZERO
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> ZERO();
/// dxfg_v4_TimePeriod_UNLIMITED
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> UNLIMITED();
/// dxfg_v4_TimePeriod_valueOf
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::int64_t value);
/// dxfg_v4_TimePeriod_valueOfString
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::string_view value);
/// dxfg_v4_TimePeriod_getTime
std::int64_t getTime(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod);
/// dxfg_v4_TimePeriod_getSeconds
std::int32_t getSeconds(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod);
/// dxfg_v4_TimePeriod_getNanos
std::int64_t getNanos(/* dxfg_v4_time_period_t* */ const JavaObjectHandle<TimePeriod> &timePeriod);

} // namespace isolated::util::IsolatedTimePeriod

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
