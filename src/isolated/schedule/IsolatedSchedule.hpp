// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/schedule/Schedule.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSchedule {

/* dxfg_v4_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile);

/* dxfg_v4_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule> getInstance(std::string_view scheduleDefinition);

/* dxfg_v4_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile,
            std::string_view venue);

std::vector<std::string>
getTradingVenues(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile);

void downloadDefaults(std::string_view downloadConfig);

void setDefaults(const std::vector<char> &data);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
getSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayById(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int32_t dayId);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByYearMonthDay(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule,
                     std::int32_t yearMonthDay);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
getNearestSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                        /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
findNearestSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                         /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

std::string getName(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

std::string getTimeZoneDisplayName(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

std::string getTimeZoneId(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

// int32_t dxfg_v4_Schedule_getDefinition(graal_isolatethread_t *thread, dxfg_v4_schedule_t* schedule, DXFG_OUT char
// **definition);
std::string getDefinition(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

} // namespace IsolatedSchedule

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()