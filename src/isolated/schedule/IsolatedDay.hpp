// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/schedule/Day.hpp"
#include "../../../include/dxfeed_graal_cpp_api/schedule/DayFilter.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/* dxfg_v4_day_filter_t* */ JavaObjectHandle<dxfcpp::DayFilter> getInstance(std::uint32_t code);

}

namespace IsolatedDay {

/* dxfg_v4_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getSchedule(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayId(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getYearMonthDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getYear(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getMonthOfYear(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayOfMonth(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayOfWeek(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isHoliday(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isShortDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isTrading(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int64_t getStartTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int64_t getEndTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool containsTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day, std::int64_t time);

std::int64_t getResetTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

/* dxfg_v4_session_list* */ void *getSessions(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
getSessionByTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day, std::int64_t time);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
getFirstSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
getLastSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
               /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
findFirstSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                 /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_v4_session_t* */ JavaObjectHandle<dxfcpp::Session>
findLastSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
getPrevDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
           /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
getNextDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
           /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
findPrevDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_v4_day_t* */ JavaObjectHandle<dxfcpp::Day>
findNextDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

std::size_t hashCode(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool equals(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &otherDay);

std::string toString(/* dxfg_v4_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

} // namespace IsolatedDay

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()