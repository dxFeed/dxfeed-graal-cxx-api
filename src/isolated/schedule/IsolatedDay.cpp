// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedDay.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/* dxfg_day_filter_t* */ JavaObjectHandle<dxfcpp::DayFilter> getInstance(std::uint32_t code) {
    return JavaObjectHandle<dxfcpp::DayFilter>{
        runGraalFunctionAndThrowIfNullptr(dxfg_DayFilter_getInstance, static_cast<dxfg_day_filter_prepare_t>(code))};
}

} // namespace IsolatedDayFilter

namespace IsolatedDay {

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getSchedule(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getSchedule`. The `day` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Schedule>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_getSchedule, static_cast<dxfg_day_t *>(day.get()))};
}

std::int32_t getDayId(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_getDayId`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getDayId, static_cast<dxfg_day_t *>(day.get()));
}

std::int32_t getYearMonthDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getYearMonthDay`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getYearMonthDay, static_cast<dxfg_day_t *>(day.get()));
}

std::int32_t getYear(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_getYear`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getYear, static_cast<dxfg_day_t *>(day.get()));
}

std::int32_t getMonthOfYear(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getMonthOfYear`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getMonthOfYear, static_cast<dxfg_day_t *>(day.get()));
}

std::int32_t getDayOfMonth(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getDayOfMonth`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getDayOfMonth, static_cast<dxfg_day_t *>(day.get()));
}

std::int32_t getDayOfWeek(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getDayOfWeek`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getDayOfWeek, static_cast<dxfg_day_t *>(day.get()));
}

bool isHoliday(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_isHoliday`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_isHoliday, static_cast<dxfg_day_t *>(day.get())) == 1;
}

bool isShortDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_isShortDay`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_isShortDay, static_cast<dxfg_day_t *>(day.get())) == 1;
}

bool isTrading(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_isTrading`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_isTrading, static_cast<dxfg_day_t *>(day.get())) == 1;
}

std::int64_t getStartTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getStartTime`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getStartTime, static_cast<dxfg_day_t *>(day.get()));
}

std::int64_t getEndTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_getEndTime`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getEndTime, static_cast<dxfg_day_t *>(day.get()));
}

bool containsTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day, std::int64_t time) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_containsTime`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_containsTime, static_cast<dxfg_day_t *>(day.get()), time) == 1;
}

std::int64_t getResetTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getResetTime`. The `day` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_getResetTime, static_cast<dxfg_day_t *>(day.get()));
}

/* dxfg_session_list* */ void *getSessions(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getSessions`. The `day` handle is invalid");
    }

    return static_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_getSessions, static_cast<dxfg_day_t *>(day.get())));
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getSessionByTime(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day, std::int64_t time) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getSessionByTime`. The `day` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_getSessionByTime, static_cast<dxfg_day_t *>(day.get()), time)};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getFirstSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getFirstSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getFirstSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_getFirstSession, static_cast<dxfg_day_t *>(day.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getLastSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getLastSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getLastSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_getLastSession, static_cast<dxfg_day_t *>(day.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findFirstSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                 /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findFirstSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findFirstSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_findFirstSession, static_cast<dxfg_day_t *>(day.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findLastSession(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findLastSession`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findLastSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_findLastSession, static_cast<dxfg_day_t *>(day.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getPrevDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
           /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_getPrevDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getPrevDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Day_getPrevDay, static_cast<dxfg_day_t *>(day.get()), static_cast<dxfg_day_filter_t *>(filter.get()))};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getNextDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
           /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_getNextDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_getNextDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Day_getNextDay, static_cast<dxfg_day_t *>(day.get()), static_cast<dxfg_day_filter_t *>(filter.get()))};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
findPrevDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findPrevDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findPrevDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Day_findPrevDay, static_cast<dxfg_day_t *>(day.get()), static_cast<dxfg_day_filter_t *>(filter.get()))};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
findNextDay(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_day_filter_t* */ const JavaObjectHandle<dxfcpp::DayFilter> &filter) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findNextDay`. The `day` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_findNextDay`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Day_findNextDay, static_cast<dxfg_day_t *>(day.get()), static_cast<dxfg_day_filter_t *>(filter.get()))};
}

std::size_t hashCode(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_hashCode`. The `day` handle is invalid");
    }

    return static_cast<std::size_t>(
        runGraalFunctionAndThrowIfMinusOne(dxfg_Day_hashCode, static_cast<dxfg_day_t *>(day.get())));
}

bool equals(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day,
            /* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &otherDay) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_equals`. The `day` handle is invalid");
    }

    if (!otherDay) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Day_equals`. The `otherDay` handle is invalid");
    }

    if (day.get() == otherDay.get()) {
        return true;
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Day_equals, static_cast<dxfg_day_t *>(day.get()),
                                              static_cast<dxfg_day_t *>(otherDay.get())) == 1;
}

std::string toString(/* dxfg_day_t* */ const JavaObjectHandle<dxfcpp::Day> &day) {
    if (!day) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Day_toString`. The `day` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalFunctionAndThrowIfNullptr(dxfg_Day_toString, static_cast<dxfg_day_t *>(day.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

} // namespace IsolatedDay

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE