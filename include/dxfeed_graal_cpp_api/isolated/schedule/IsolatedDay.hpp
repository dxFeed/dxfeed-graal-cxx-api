// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../schedule/Day.hpp"
#include "../../schedule/DayFilter.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/* dxfg_day_filter_t* */ JavaObjectHandle<dxfcpp::DayFilter> getInstance(std::uint32_t code);

}

namespace IsolatedDay {

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule> getSchedule(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayId(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getYearMonthDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getYear(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getMonthOfYear(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayOfMonth(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int32_t getDayOfWeek(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isHoliday(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isShortDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool isTrading(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int64_t getStartTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

std::int64_t getEndTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool containsTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day, std::int64_t time);

std::int64_t getResetTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

/* dxfg_session_list* */ void *getSessions(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session> getSessionByTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                                             std::int64_t time);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getFirstSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getLastSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findFirstSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                 /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findLastSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day> getPrevDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                                   /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day> getNextDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                                   /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day> findPrevDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                                    /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day> findNextDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                                    /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter);

std::size_t getHashCode(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

bool equals(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &otherDay);

std::string toString(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day);

} // namespace IsolatedDay

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()