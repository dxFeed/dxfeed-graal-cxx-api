// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedSchedule.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSchedule {

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile) {
    if (!instrumentProfile) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getInstance`. The `instrumentProfile` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Schedule>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getInstance, static_cast<dxfg_instrument_profile_t *>(instrumentProfile.get()))};
}

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule> getInstance(const StringLikeWrapper &scheduleDefinition) {
    return JavaObjectHandle<dxfcpp::Schedule>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Schedule_getInstance2, scheduleDefinition.c_str())};
}

/* dxfg_schedule_t* */ JavaObjectHandle<dxfcpp::Schedule>
getInstance(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile,
            const StringLikeWrapper &venue) {
    if (!instrumentProfile) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getInstance3`. The `instrumentProfile` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Schedule>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getInstance3, static_cast<dxfg_instrument_profile_t *>(instrumentProfile.get()), venue.c_str())};
}

std::vector<std::string> getTradingVenues(
    /* dxfg_instrument_profile_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &instrumentProfile) {
    if (!instrumentProfile) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getTradingVenues`. The `instrumentProfile` handle is invalid");
    }

    auto venues = isolated::internal::IsolatedStringList::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getTradingVenues, static_cast<dxfg_instrument_profile_t *>(instrumentProfile.get())));

    std::vector<std::string> result{};

    for (auto i = 0; i < static_cast<dxfg_string_list *>(venues.get())->size; i++) {
        result.push_back(dxfcpp::toString(static_cast<dxfg_string_list *>(venues.get())->elements[i]));
    }

    return result;
}

void downloadDefaults(const StringLikeWrapper &downloadConfig) {
    runGraalFunctionAndThrowIfMinusOne(dxfg_Schedule_downloadDefaults, downloadConfig.c_str());
}

void setDefaults(const std::vector<char> &data) {
    runGraalFunctionAndThrowIfMinusOne(dxfg_Schedule_setDefaults, data.data(), fitToType<std::int32_t>(data.size()));
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getSessionByTime`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getSessionByTime, static_cast<dxfg_schedule_t *>(schedule.get()), time)};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getDayByTime`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getDayByTime, static_cast<dxfg_schedule_t *>(schedule.get()), time)};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayById(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int32_t dayId) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getDayById`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getDayById, static_cast<dxfg_schedule_t *>(schedule.get()), dayId)};
}

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDayByYearMonthDay(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule,
                     std::int32_t yearMonthDay) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getDayByYearMonthDay`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getDayByYearMonthDay, static_cast<dxfg_schedule_t *>(schedule.get()), yearMonthDay)};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getNearestSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                        /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getNearestSessionByTime`. The `schedule` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getNearestSessionByTime`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_getNearestSessionByTime, static_cast<dxfg_schedule_t *>(schedule.get()), time,
        static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findNearestSessionByTime(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule, std::int64_t time,
                         /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_findNearestSessionByTime`. The `schedule` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_findNearestSessionByTime`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{runGraalFunctionAndThrowIfNullptr(
        dxfg_Schedule_findNearestSessionByTime, static_cast<dxfg_schedule_t *>(schedule.get()), time,
        static_cast<dxfg_session_filter_t *>(filter.get()))};
}

std::string getName(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getName`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalFunctionAndThrowIfNullptr(dxfg_Schedule_getName, static_cast<dxfg_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

std::string getTimeZoneDisplayName(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getTimeZone`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalFunctionAndThrowIfNullptr(dxfg_Schedule_getTimeZone, static_cast<dxfg_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

std::string getTimeZoneId(/* dxfg_schedule_t* */ const JavaObjectHandle<dxfcpp::Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Schedule_getTimeZone_getID`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalFunctionAndThrowIfNullptr(dxfg_Schedule_getTimeZone_getID, static_cast<dxfg_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}


} // namespace IsolatedSchedule

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE