// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../util/TimePeriod.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::util::IsolatedTimePeriod {

/*

dxfg_time_period_t*         dxfg_TimePeriod_ZERO(graal_isolatethread_t *thread);
dxfg_time_period_t*         dxfg_TimePeriod_UNLIMITED(graal_isolatethread_t *thread);
dxfg_time_period_t*         dxfg_TimePeriod_valueOf(graal_isolatethread_t *thread, int64_t value);
dxfg_time_period_t*         dxfg_TimePeriod_valueOf2(graal_isolatethread_t *thread, const char* value);
int64_t                     dxfg_TimePeriod_getTime(graal_isolatethread_t *thread, dxfg_time_period_t* timePeriod);
int32_t                     dxfg_TimePeriod_getSeconds(graal_isolatethread_t *thread, dxfg_time_period_t* timePeriod);
int64_t                     dxfg_TimePeriod_getNanos(graal_isolatethread_t *thread, dxfg_time_period_t* timePeriod);

*/

/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> ZERO();
/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> UNLIMITED();
/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::int64_t value);
/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod> valueOf(std::string_view value);
std::int64_t getTime(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod>& timePeriod);
std::int32_t getSeconds(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod>& timePeriod);
std::int64_t getNanos(/* dxfg_time_period_t* */ const JavaObjectHandle<TimePeriod>& timePeriod);

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()

