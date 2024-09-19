// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../schedule/Schedule.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSchedule {

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile);

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule> getInstance(const StringLikeWrapper &scheduleDefinition);

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile,
            const StringLikeWrapper &venue);

std::vector<std::string>
getTradingVenues(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile);

void downloadDefaults(const StringLikeWrapper &downloadConfig);

void setDefaults(const std::vector<char> &data);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayById(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int32_t dayId);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByYearMonthDay(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule,
                     std::int32_t yearMonthDay);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getNearestSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                        /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findNearestSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                         /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

std::string getName(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

std::string getTimeZoneDisplayName(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

std::string getTimeZoneId(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule);

} // namespace IsolatedSchedule

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()