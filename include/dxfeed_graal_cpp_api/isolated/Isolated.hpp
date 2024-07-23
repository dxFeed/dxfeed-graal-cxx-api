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

namespace ipf {
struct InstrumentProfileCollector {
    static /* dxfg_ipf_collector_t* */ void *create() noexcept;
    static std::int64_t getLastUpdateTime(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept;
    static bool updateInstrumentProfile(
        /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
        /* dxfg_instrument_profile_t * */ void *ip) noexcept;
    static /* dxfg_iterable_ip_t* */ void *
    view(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept;
    static bool addUpdateListener(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
                                  /* dxfg_ipf_update_listener_t* */ void *listener) noexcept;
    static bool removeUpdateListener(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
                                     /* dxfg_ipf_update_listener_t* */ void *listener) noexcept;
};

struct InstrumentProfileConnection {
    static /* dxfg_ipf_connection_t* */ void *
    createConnection(const std::string &address,
                     /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept;
    static std::string getAddress(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static std::int64_t getUpdatePeriod(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static bool setUpdatePeriod(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
                                std::int64_t updatePeriod) noexcept;
    static dxfcpp::InstrumentProfileConnection::State
    getState(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static std::int64_t getLastModified(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static bool start(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static bool close(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept;
    static bool addStateChangeListener(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
                                       /* dxfg_ipf_connection_state_change_listener_t * */ void *listener) noexcept;
    static bool waitUntilCompleted(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
                                   std::int64_t timeoutInMs) noexcept;
};

struct IpfPropertyChangeListener {
    static /* dxfg_ipf_connection_state_change_listener_t* */ void *
    create(/* dxfg_ipf_connection_state_change_listener_func */ void *userFunc, void *userData) noexcept;
};

struct InstrumentProfileIterator {
    static bool hasNext(/* dxfg_iterable_ip_t * */ void *iterable) noexcept;
    static /* dxfg_instrument_profile_t* */ void *next(/* dxfg_iterable_ip_t * */ void *iterable) noexcept;
};

struct InstrumentProfileUpdateListener {
    static /* dxfg_ipf_update_listener_t* */ void *create(/* dxfg_ipf_update_listener_function */ void *userFunc,
                                                          void *userData) noexcept;
};

} // namespace ipf

namespace ondemand {

struct OnDemandService {
    static /* dxfg_on_demand_service_t* */ void *getInstance(/* dxfg_endpoint_t * */ void *endpoint) noexcept;
    static bool isReplaySupported(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool isReplay(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool isClear(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static std::int64_t getTime(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static double getSpeed(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool replay(/* dxfg_on_demand_service_t * */ void *service, std::int64_t time) noexcept;
    static bool replay(/* dxfg_on_demand_service_t * */ void *service, std::int64_t time, double speed) noexcept;
    static bool pause(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool stopAndResume(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool stopAndClear(/* dxfg_on_demand_service_t * */ void *service) noexcept;
    static bool setSpeed(/* dxfg_on_demand_service_t * */ void *service, double speed) noexcept;
};

} // namespace ondemand

namespace schedule {
struct DayFilter {
    static /* dxfg_day_filter_t* */ void *getInstance(std::uint32_t code) noexcept;
};

struct Day {
    static /* dxfg_schedule_t* */ void *getSchedule(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getDayId(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getYearMonthDay(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getYear(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getMonthOfYear(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getDayOfMonth(/* dxfg_day_t* */ void *day) noexcept;
    static std::int32_t getDayOfWeek(/* dxfg_day_t* */ void *day) noexcept;
    static bool isHoliday(/* dxfg_day_t* */ void *day) noexcept;
    static bool isShortDay(/* dxfg_day_t* */ void *day) noexcept;
    static bool isTrading(/* dxfg_day_t* */ void *day) noexcept;
    static std::int64_t getStartTime(/* dxfg_day_t* */ void *day) noexcept;
    static std::int64_t getEndTime(/* dxfg_day_t* */ void *day) noexcept;
    static bool containsTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept;
    static std::int64_t getResetTime(/* dxfg_day_t* */ void *day) noexcept;
    static /* dxfg_session_list* */ void* getSessions(/* dxfg_day_t* */ void *day);
    static /* dxfg_session_t* */ void *getSessionByTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept;
    static /* dxfg_session_t* */ void *getFirstSession(/* dxfg_day_t* */ void *day,
                                                       /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *getLastSession(/* dxfg_day_t* */ void *day,
                                                      /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *findFirstSession(/* dxfg_day_t* */ void *day,
                                                        /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *findLastSession(/* dxfg_day_t* */ void *day,
                                                       /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_day_t* */ void *getPrevDay(/* dxfg_day_t* */ void *day,
                                              /* dxfg_day_filter_t* */ void *filter) noexcept;
    static /* dxfg_day_t* */ void *getNextDay(/* dxfg_day_t* */ void *day,
                                              /* dxfg_day_filter_t* */ void *filter) noexcept;
    static /* dxfg_day_t* */ void *findPrevDay(/* dxfg_day_t* */ void *day,
                                               /* dxfg_day_filter_t* */ void *filter) noexcept;
    static /* dxfg_day_t* */ void *findNextDay(/* dxfg_day_t* */ void *day,
                                               /* dxfg_day_filter_t* */ void *filter) noexcept;
    static std::size_t getHashCode(/* dxfg_day_t* */ void *day) noexcept;
    static bool equals(/* dxfg_day_t* */ void *day, /* dxfg_day_t* */ void *otherDay) noexcept;
    static std::string toString(/* dxfg_day_t* */ void *day) noexcept;
};

struct SessionFilter {
    static /* dxfg_session_filter_t* */ void *getInstance(std::uint32_t code) noexcept;
};

struct Session {
    static /* dxfg_day_t* */ void *getDay(/* dxfg_session_t* */ void *session) noexcept;
    static std::int32_t getType(/* dxfg_session_t* */ void *session) noexcept;
    static bool isTrading(/* dxfg_session_t* */ void *session) noexcept;
    static bool isEmpty(/* dxfg_session_t* */ void *session) noexcept;
    static std::int64_t getStartTime(/* dxfg_session_t* */ void *session) noexcept;
    static std::int64_t getEndTime(/* dxfg_session_t* */ void *session) noexcept;
    static bool containsTime(/* dxfg_session_t* */ void *session, std::int64_t time) noexcept;
    static /* dxfg_session_t* */ void *getPrevSession(/* dxfg_session_t* */ void *session,
                                                      /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *getNextSession(/* dxfg_session_t* */ void *session,
                                                      /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *findPrevSession(/* dxfg_session_t* */ void *session,
                                                       /* dxfg_session_filter_t* */ void *filter) noexcept;
    static /* dxfg_session_t* */ void *findNextSession(/* dxfg_session_t* */ void *session,
                                                       /* dxfg_session_filter_t* */ void *filter) noexcept;
    static std::size_t getHashCode(/* dxfg_session_t* */ void *session) noexcept;
    static bool equals(/* dxfg_session_t* */ void *session, /* dxfg_session_t* */ void *otherSession) noexcept;
    static std::string toString(/* dxfg_session_t* */ void *session) noexcept;
};

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