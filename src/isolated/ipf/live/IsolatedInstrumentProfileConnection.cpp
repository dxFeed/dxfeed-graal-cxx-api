// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/ipf/live/IsolatedInstrumentProfileConnection.hpp>

DXFCPP_BEGIN_NAMESPACE

dxfcpp::InstrumentProfileConnection::State graalIpfConnectionStateToState(dxfg_ipf_connection_state_t state);

namespace isolated::ipf::live {
namespace IsolatedInstrumentProfileConnection {

/* dxfg_ipf_connection_t* */ JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
createConnection(const StringLikeWrapper &address,
                 /* dxfg_ipf_collector_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileCollector>
                     &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_createConnection`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::InstrumentProfileConnection>{
        runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfileConnection_createConnection, address.c_str(),
                                          static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get()))};
}

std::string getAddress(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                           &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfileConnection_getAddress`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    auto address =
        runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfileConnection_getAddress,
                                          static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()));

    auto result = dxfcpp::toString(address);

    isolated::internal::IsolatedString::release(address);

    return result;
}

std::int64_t getUpdatePeriod(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                                 &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_getUpdatePeriod`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileConnection_getUpdatePeriod,
                                              static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()));
}

bool setUpdatePeriod(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                         &instrumentProfileConnection,
                     std::int64_t updatePeriod) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_setUpdatePeriod`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileConnection_setUpdatePeriod,
                                              static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()),
                                              updatePeriod) == 0;
}

dxfcpp::InstrumentProfileConnection::State
getState(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
             &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfileConnection_getState`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    return graalIpfConnectionStateToState(
        runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileConnection_getState,
                                           static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get())));
}

std::int64_t getLastModified(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                                 &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_getLastModified`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileConnection_getLastModified,
                                              static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()));
}

bool start(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfileConnection_start`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(
               dxfg_InstrumentProfileConnection_start,
               static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get())) == 0;
}

bool close(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
               &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfileConnection_close`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(
               dxfg_InstrumentProfileConnection_close,
               static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get())) == 0;
}

bool addStateChangeListener(
    /* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
        &instrumentProfileConnection,
    /* dxfg_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::IpfPropertyChangeListener>
        &listener) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_addStateChangeListener`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_addStateChangeListener`. The "
            "`listener` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(
               dxfg_InstrumentProfileConnection_addStateChangeListener,
               static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()),
               static_cast<dxfg_ipf_connection_state_change_listener_t *>(listener.get())) == 0;
}

bool waitUntilCompleted(/* dxfg_ipf_connection_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfileConnection>
                            &instrumentProfileConnection,
                        std::int64_t timeoutInMs) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileConnection_waitUntilCompleted`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileConnection_waitUntilCompleted,
                                              static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnection.get()),
                                              timeoutInMs) == 1;
}

} // namespace IsolatedInstrumentProfileConnection
} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE