// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/TimeFormat.hpp"

#include "../api/DXEndpoint.hpp"

#include "../ipf/live/InstrumentProfileConnection.hpp"

#include <cstdint>
#include <string_view>
#include <string>
#include <unordered_set>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated {

namespace schedule {

struct Schedule {
    static /* dxfg_schedule_t* */ void *getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept;
    static /* dxfg_schedule_t* */ void *getInstance(const std::string &scheduleDefinition) noexcept;
    static /* dxfg_schedule_t* */ void *getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile,
                                                    const std::string &venue) noexcept;
    static std::vector<std::string> getTradingVenues(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept;
    static void downloadDefaults(const std::string &downloadConfig) noexcept;
    static bool setDefaults(const std::vector<char> &data) noexcept;
    static /* dxfg_session_t* */ void *getSessionByTime(/* dxfg_schedule_t* */ void *schedule,
                                                        std::int64_t time) noexcept;
    static /* dxfg_day_t* */ void *getDayByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time) noexcept;
    static /* dxfg_day_t* */ void *getDayById(/* dxfg_schedule_t* */ void *schedule, std::int32_t dayId) noexcept;
    static /* dxfg_day_t* */ void *getDayByYearMonthDay(/* dxfg_schedule_t* */ void *schedule,
                                                        std::int32_t yearMonthDay) noexcept;
    static /* dxfg_session_t* */ void *getNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                               /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *findNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule,
                                                                std::int64_t time,
                                                                /* dxfg_session_filter_t* */ void *filter) noexcept;
    static std::string getName(/* dxfg_schedule_t* */ void *schedule) noexcept;
    static std::string getTimeZoneDisplayName(/* dxfg_schedule_t* */ void *schedule) noexcept;
    static std::string getTimeZoneId(/* dxfg_schedule_t* */ void *schedule) noexcept;
};
} // namespace schedule
} // namespace isolated

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()