// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../../include/dxfeed_graal_cpp_api/ipf/live/InstrumentProfileCollector.hpp"
#include "../../../../include/dxfeed_graal_cpp_api/ipf/live/InstrumentProfileConnection.hpp"

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live {
namespace IsolatedInstrumentProfileConnection {

/* dxfg_v4_ipf_connection_t* */ JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
createConnection(std::string_view address,
                 /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileCollector>
                     &instrumentProfileCollector);

std::string getAddress(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                           &instrumentProfileConnection);

std::int64_t
getUpdatePeriod(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                    &instrumentProfileConnection);

void setUpdatePeriod(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                         &instrumentProfileConnection,
                     std::int64_t updatePeriod);

dxfcpp::InstrumentProfileConnection::State
getState(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
             &instrumentProfileConnection);

std::int64_t
getLastModified(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                    &instrumentProfileConnection);

void start(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection);

void close(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection);

void addStateChangeListener(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
        &instrumentProfileConnection,
    /* dxfg_v4_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
        &listener);

/**
 * Removes a native state-change listener from an instrument-profile connection.
 *
 * @param instrumentProfileConnection The native connection handle.
 * @param listener The native listener handle.
 * @throws JavaException if the SDK reports an error.
 */
void removeStateChangeListener(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
        &instrumentProfileConnection,
    /* dxfg_v4_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
        &listener);

bool waitUntilCompleted(/* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                            &instrumentProfileConnection,
                        std::int64_t timeoutInMs);

} // namespace IsolatedInstrumentProfileConnection

namespace IsolatedIpfPropertyChangeListener {

/* dxfg_v4_ipf_connection_state_change_listener_t* */ JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
create(/* dxfg_v4_ipf_connection_state_change_listener_func */ void *userFunc, void *userData);
}

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
