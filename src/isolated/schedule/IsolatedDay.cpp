// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDay.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/// dxfg_v4_DayFilter_getInstance
/* dxfg_v4_day_filter_t* */ JavaObjectHandle<DayFilter> getInstance(std::uint32_t code) {
    return JavaObjectHandle<DayFilter>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DayFilter_getInstance, static_cast<dxfg_v4_day_filter_prepare_t>(code))};
}

} // namespace IsolatedDayFilter

namespace IsolatedDay {

/// dxfg_v4_Day_getSchedule
/* dxfg_v4_schedule_t* */ JavaObjectHandle<Schedule>
getSchedule(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getSchedule`. The `day` handle is invalid");
    }

    return JavaObjectHandle<Schedule>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getSchedule, static_cast<dxfg_v4_day_t *>(day.get()))};
}

/// dxfg_v4_Day_getDayId
std::int32_t getDayId(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getDayId`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getDayId, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getYearMonthDay
std::int32_t getYearMonthDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getYearMonthDay`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getYearMonthDay, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getYear
std::int32_t getYear(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getYear`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getYear, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getMonthOfYear
std::int32_t getMonthOfYear(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getMonthOfYear`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getMonthOfYear, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getDayOfMonth
std::int32_t getDayOfMonth(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getDayOfMonth`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getDayOfMonth, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getDayOfWeek
std::int32_t getDayOfWeek(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getDayOfWeek`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getDayOfWeek, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_isHoliday
bool isHoliday(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_isHoliday`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_isHoliday, static_cast<dxfg_v4_day_t *>(day.get())) != 0;
}

/// dxfg_v4_Day_isShortDay
bool isShortDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_isShortDay`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_isShortDay, static_cast<dxfg_v4_day_t *>(day.get())) != 0;
}

/// dxfg_v4_Day_isTrading
bool isTrading(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_isTrading`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_isTrading, static_cast<dxfg_v4_day_t *>(day.get())) != 0;
}

/// dxfg_v4_Day_getStartTime
std::int64_t getStartTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getStartTime`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getStartTime, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getEndTime
std::int64_t getEndTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getEndTime`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getEndTime, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_containsTime
bool containsTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day, std::int64_t time) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_containsTime`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_containsTime, static_cast<dxfg_v4_day_t *>(day.get()), time) != 0;
}

/// dxfg_v4_Day_getResetTime
std::int64_t getResetTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getResetTime`. The `day` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_getResetTime, static_cast<dxfg_v4_day_t *>(day.get()));
}

/// dxfg_v4_Day_getSessions
/* dxfg_v4_session_list* */ void *getSessions(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getSessions`. The `day` handle is invalid");
    }

    return static_cast<void *>(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getSessions, static_cast<dxfg_v4_day_t *>(day.get())));
}

/// dxfg_v4_Day_getSessionByTime
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getSessionByTime(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day, std::int64_t time) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getSessionByTime`. The `day` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Day_getSessionByTime, static_cast<dxfg_v4_day_t *>(day.get()), time)};
}

/// dxfg_v4_Day_getFirstSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getFirstSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getFirstSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getFirstSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getFirstSession, static_cast<dxfg_v4_day_t *>(day.get()),
                                            static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_getLastSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getLastSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
               /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getLastSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getLastSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getLastSession, static_cast<dxfg_v4_day_t *>(day.get()),
                                            static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_findFirstSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
findFirstSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
                 /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findFirstSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findFirstSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithResult(dxfg_v4_Day_findFirstSession, static_cast<dxfg_v4_day_t *>(day.get()),
                                     static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_findLastSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
findLastSession(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findLastSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findLastSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithResult(dxfg_v4_Day_findLastSession, static_cast<dxfg_v4_day_t *>(day.get()),
                                     static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_getPrevDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getPrevDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
           /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getPrevDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getPrevDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Day>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getPrevDay, static_cast<dxfg_v4_day_t *>(day.get()),
                                            static_cast<dxfg_v4_day_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_getNextDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getNextDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
           /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getNextDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_getNextDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Day>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_getNextDay, static_cast<dxfg_v4_day_t *>(day.get()),
                                            static_cast<dxfg_v4_day_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_findPrevDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
findPrevDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
            /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findPrevDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findPrevDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Day>{
        runGraalV4FunctionWithResult(dxfg_v4_Day_findPrevDay, static_cast<dxfg_v4_day_t *>(day.get()),
                                     static_cast<dxfg_v4_day_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_findNextDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
findNextDay(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
            /* dxfg_v4_day_filter_t* */ const JavaObjectHandle<DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findNextDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_findNextDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Day>{
        runGraalV4FunctionWithResult(dxfg_v4_Day_findNextDay, static_cast<dxfg_v4_day_t *>(day.get()),
                                     static_cast<dxfg_v4_day_filter_t *>(filter.get()))};
}

/// dxfg_v4_Day_hashCode
std::size_t hashCode(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_hashCode`. The `day` handle is invalid");
    }

    return static_cast<std::size_t>(
        runGraalV4FunctionWithResult(dxfg_v4_Day_hashCode, static_cast<dxfg_v4_day_t *>(day.get())));
}

/// dxfg_v4_Day_equals
bool equals(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day,
            /* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &otherDay) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_equals`. The `day` handle is invalid");
    }

    if (!otherDay) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_equals`. The `otherDay` handle is invalid");
    }

    if (day.get() == otherDay.get()) {
        return true;
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Day_equals, static_cast<dxfg_v4_day_t *>(day.get()),
                                        static_cast<dxfg_v4_day_t *>(otherDay.get())) != 0;
}

/// dxfg_v4_Day_toString
std::string toString(/* dxfg_v4_day_t* */ const JavaObjectHandle<Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Day_toString`. The `day` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Day_toString, static_cast<dxfg_v4_day_t *>(day.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

} // namespace IsolatedDay

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE