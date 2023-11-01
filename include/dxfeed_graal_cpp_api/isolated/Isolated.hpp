// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp::isolated {
struct String {
    static bool release(const char *string) noexcept;
};

struct StringList {
    static bool release(/* dxfg_string_list* */ void *stringList) noexcept;
};

namespace api {
struct DXEndpoint {
    static bool close(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle) noexcept;
    static dxfcpp::DXEndpoint::State getState(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle) noexcept;
    static bool user(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &user) noexcept;
    static bool password(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &password) noexcept;
    static bool connect(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &address) noexcept;
};
} // namespace api

namespace ipf {
struct InstrumentProfileReader {
    static /* dxfg_instrument_profile_reader_t* */ void *create() noexcept;

    static std::int64_t
    getLastModified(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept;

    static bool wasComplete(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept;

    static /* dxfg_instrument_profile_list* */ void *
    readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                 const std::string &address) noexcept;

    static /* dxfg_instrument_profile_list* */ void *
    readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                 const std::string &address, const std::string &user, const std::string &password) noexcept;

    static std::string resolveSourceURL(const std::string &address) noexcept;
};

struct InstrumentProfileCollector {
    static /* dxfg_ipf_collector_t* */ void *create() noexcept;
    static std::int64_t getLastUpdateTime(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept;
    static bool updateInstrumentProfile(
        /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
        /* dxfg_instrument_profile_t * */ void *ip) noexcept;
    static bool addUpdateListener(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
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

struct InstrumentProfileList {
    static bool release(/* dxfg_instrument_profile_list * */ void *graalInstrumentProfileList) noexcept;
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

namespace schedule {
struct DayFilter {
    static /* dxfg_day_filter_t* */ void *getInstance(std::uint32_t code) noexcept;
};

struct Day {
    static std::int32_t getYearMonthDay(/* dxfg_day_t* */ void *day) noexcept;
    static /* dxfg_day_t* */ void *findNextDay(/* dxfg_day_t* */ void *day,
                                               /* dxfg_day_filter_t* */ void *filter) noexcept;
    static std::string toString(/* dxfg_day_t* */ void *day) noexcept;
};

struct SessionFilter {
    static /* dxfg_session_filter_t* */ void *getInstance(std::uint32_t code) noexcept;
};

struct Session {
    static /* dxfg_day_t* */ void *getDay(/* dxfg_session_t* */ void *session) noexcept;
    static bool isTrading(/* dxfg_session_t* */ void *session) noexcept;
    static /* dxfg_session_t* */ void *getNextSession(/* dxfg_session_t* */ void *session,
                                                      /* dxfg_session_filter_t* */ void *filter) noexcept;
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
    static /* dxfg_session_t* */ void *getNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                               /* dxfg_session_filter_t* */ void *filter) noexcept;
};
} // namespace schedule
} // namespace dxfcpp::isolated