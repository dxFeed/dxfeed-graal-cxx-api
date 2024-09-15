// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../internal/Conf.hpp"

#include "../../../ipf/live/InstrumentProfileCollector.hpp"
#include "../../../ipf/live/InstrumentProfileConnection.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live {
namespace IsolatedInstrumentProfileConnection {

/* dxfg_ipf_connection_t* */ JavaObjectHandle<dxfcpp::InstrumentProfileConnection> createConnection(
    const StringLikeWrapper &address,
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileCollector> &instrumentProfileCollector);

std::string getAddress(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                           &instrumentProfileConnection);

std::int64_t getUpdatePeriod(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                                 &instrumentProfileConnection);

bool setUpdatePeriod(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                         &instrumentProfileConnection,
                     std::int64_t updatePeriod);

dxfcpp::InstrumentProfileConnection::State
getState(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
             &instrumentProfileConnection);

std::int64_t getLastModified(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                                 &instrumentProfileConnection);

bool start(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection);

bool close(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection);

bool addStateChangeListener(
    /* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
        &instrumentProfileConnection,
    /* dxfg_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
        &listener);

bool waitUntilCompleted(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                            &instrumentProfileConnection,
                        std::int64_t timeoutInMs);

} // namespace IsolatedInstrumentProfileConnection

namespace IsolatedIpfPropertyChangeListener {

/* dxfg_ipf_connection_state_change_listener_t* */ JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
create(/* dxfg_ipf_connection_state_change_listener_func */ void *userFunc, void *userData);

}

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()