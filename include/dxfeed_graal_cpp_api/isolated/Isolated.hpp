// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp::isolated {
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

    static std::int64_t getLastModified(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept;

    static bool wasComplete(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept;

    static /* dxfg_instrument_profile_list* */ void *
    readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                 const std::string &address) noexcept;

    static /* dxfg_instrument_profile_list* */ void *
    readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                 const std::string &address, const std::string &user, const std::string &password) noexcept;

    static std::string resolveSourceURL(const std::string & address) noexcept;
};

struct InstrumentProfileCollector {
    static /* dxfg_ipf_collector_t* */ void *create() noexcept;
};

struct InstrumentProfileConnection {
    static /* dxfg_ipf_connection_t* */ void* createConnection(const std::string& address, /* dxfg_ipf_collector_t* */ void* instrumentProfileCollectorHandle) noexcept;
    static std::string getAddress(/* dxfg_ipf_connection_t * */ void* instrumentProfileConnectionHandle) noexcept;
    static std::int64_t getUpdatePeriod(/* dxfg_ipf_connection_t * */ void* instrumentProfileConnectionHandle) noexcept;
    static bool setUpdatePeriod(/* dxfg_ipf_connection_t * */ void* instrumentProfileConnectionHandle, std::int64_t updatePeriod);
};

struct InstrumentProfileList {
    static bool release(/* dxfg_instrument_profile_list * */ void *graalInstrumentProfileList) noexcept;
};

} // namespace ipf
} // namespace dxfcpp::isolated