// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedSchedule.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSchedule {

/// dxfg_v4_Schedule_getInstanceByProfileHandle
/* dxfg_v4_schedule_t* */ JavaObjectHandle<Schedule>
getInstance(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile) {
    if (!instrumentProfile) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Schedule_getInstanceByProfileHandle`. The "
                                       "`instrumentProfile` handle is invalid");
    }

    return JavaObjectHandle<Schedule>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getInstanceByProfileHandle,
        static_cast<dxfg_v4_instrument_profile_t *>(instrumentProfile.get()))};
}

/// dxfg_v4_Schedule_getInstanceByDefinition
/* dxfg_v4_schedule_t* */ JavaObjectHandle<Schedule> getInstance(std::string_view scheduleDefinition) {
    return JavaObjectHandle<Schedule>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getInstanceByDefinition,
        SdkString{scheduleDefinition, "Schedule::getInstance(scheduleDefinition)"}.c_str())};
}

/// dxfg_v4_Schedule_getInstanceByProfileHandleWithVenue
/* dxfg_v4_schedule_t* */ JavaObjectHandle<Schedule>
getInstance(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
            std::string_view venue) {
    if (!instrumentProfile) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_Schedule_getInstanceByProfileHandleWithVenue`. The "
                                       "`instrumentProfile` handle is invalid");
    }

    return JavaObjectHandle<Schedule>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getInstanceByProfileHandleWithVenue,
        static_cast<dxfg_v4_instrument_profile_t *>(instrumentProfile.get()),
        SdkString{venue, "Schedule::getInstance(venue)"}.c_str())};
}

/// dxfg_v4_Schedule_getTradingVenuesByProfileHandle
std::vector<std::string> getTradingVenues(
    /* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile) {
    if (!instrumentProfile) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Schedule_getTradingVenuesByProfileHandle`. "
                                       "The `instrumentProfile` handle is invalid");
    }

    auto venues = internal::IsolatedStringList::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getTradingVenuesByProfileHandle,
        static_cast<dxfg_v4_instrument_profile_t *>(instrumentProfile.get())));

    std::vector<std::string> result{};

    for (auto i = 0; i < static_cast<dxfg_v4_string_list *>(venues.get())->size; i++) {
        result.push_back(dxfcpp::toString(static_cast<dxfg_v4_string_list *>(venues.get())->elements[i]));
    }

    return result;
}

/// dxfg_v4_Schedule_downloadDefaults
void downloadDefaults(std::string_view downloadConfig) {
    runGraalV4Function(dxfg_v4_Schedule_downloadDefaults,
                       SdkString{downloadConfig, "Schedule::downloadDefaults(downloadConfig)"}.c_str());
}

/// dxfg_v4_Schedule_setDefaults
void setDefaults(const std::vector<char> &data) {
    runGraalV4Function(dxfg_v4_Schedule_setDefaults, data.data(), fitToType<std::int32_t>(data.size()));
}

/// dxfg_v4_Schedule_getSessionByTime
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule, std::int64_t time) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getSessionByTime`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getSessionByTime, static_cast<dxfg_v4_schedule_t *>(schedule.get()), time)};
}

/// dxfg_v4_Schedule_getDayByTime
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getDayByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule, std::int64_t time) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getDayByTime`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<Day>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getDayByTime, static_cast<dxfg_v4_schedule_t *>(schedule.get()), time)};
}

/// dxfg_v4_Schedule_getDayById
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getDayById(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule, std::int32_t dayId) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getDayById`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<Day>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getDayById, static_cast<dxfg_v4_schedule_t *>(schedule.get()), dayId)};
}

/// dxfg_v4_Schedule_getDayByYearMonthDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getDayByYearMonthDay(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule,
                     std::int32_t yearMonthDay) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getDayByYearMonthDay`. The `schedule` handle is invalid");
    }

    return JavaObjectHandle<Day>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getDayByYearMonthDay, static_cast<dxfg_v4_schedule_t *>(schedule.get()), yearMonthDay)};
}

/// dxfg_v4_Schedule_getNearestSessionByTime
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getNearestSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule, std::int64_t time,
                        /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getNearestSessionByTime`. The `schedule` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getNearestSessionByTime`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getNearestSessionByTime, static_cast<dxfg_v4_schedule_t *>(schedule.get()), time,
        static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Schedule_findNearestSessionByTime
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
findNearestSessionByTime(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule, std::int64_t time,
                         /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_findNearestSessionByTime`. The `schedule` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_findNearestSessionByTime`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithResult(
        dxfg_v4_Schedule_findNearestSessionByTime, static_cast<dxfg_v4_schedule_t *>(schedule.get()), time,
        static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Schedule_getName
std::string getName(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getName`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getName, static_cast<dxfg_v4_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

/// dxfg_v4_Schedule_getTimeZone
std::string getTimeZoneDisplayName(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getTimeZone`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getTimeZone, static_cast<dxfg_v4_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

/// dxfg_v4_Schedule_getTimeZone_getID
std::string getTimeZoneId(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getTimeZone_getID`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Schedule_getTimeZone_getID, static_cast<dxfg_v4_schedule_t *>(schedule.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

/// dxfg_v4_Schedule_getDefinition
std::string getDefinition(/* dxfg_v4_schedule_t* */ const JavaObjectHandle<Schedule> &schedule) {
    if (!schedule) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Schedule_getDefinition`. The `schedule` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_Schedule_getDefinition, static_cast<dxfg_v4_schedule_t *>(schedule.get())));

    return dxfcpp::toString(string.get());
}

} // namespace IsolatedSchedule

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE