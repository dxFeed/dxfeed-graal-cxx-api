// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedInstrumentProfileConnection.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../IsolatedCommon.hpp"
#include "../../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

InstrumentProfileConnection::State graalIpfConnectionStateToState(dxfg_ipf_connection_state_t state);

namespace isolated::ipf::live {

namespace IsolatedInstrumentProfileConnection {

/* dxfg_v4_ipf_connection_t* */ JavaObjectHandle<InstrumentProfileConnection> createConnection(
    std::string_view address,
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_createConnection`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    return JavaObjectHandle<InstrumentProfileConnection>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileConnection_createConnection,
        SdkString{address, "InstrumentProfileConnection::createConnection(address)"}.c_str(),
        static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get()))};
}

std::string getAddress(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_getAddress`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    const auto address = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileConnection_getAddress,
        static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get())));

    return dxfcpp::toString(address.get());
}

std::int64_t getUpdatePeriod(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_getUpdatePeriod`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileConnection_getUpdatePeriod,
                                        static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()));
}

void setUpdatePeriod(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection,
    std::int64_t updatePeriod) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_setUpdatePeriod`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileConnection_setUpdatePeriod,
                       static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()), updatePeriod);
}

InstrumentProfileConnection::State getState(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_InstrumentProfileConnection_getState`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    return graalIpfConnectionStateToState(
        runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileConnection_getState,
                                     static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get())));
}

std::int64_t getLastModified(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_getLastModified`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileConnection_getLastModified,
                                        static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()));
}

void start(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_InstrumentProfileConnection_start`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileConnection_start,
                       static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()));
}

void close(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_InstrumentProfileConnection_close`. The "
                                       "`instrumentProfileConnection` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileConnection_close,
                       static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()));
}

void addStateChangeListener(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection,
    /* dxfg_v4_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<IpfPropertyChangeListener>
        &listener) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_addStateChangeListener`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_addStateChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileConnection_addStateChangeListener,
                       static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()),
                       static_cast<dxfg_v4_ipf_connection_state_change_listener_t *>(listener.get()));
}

void removeStateChangeListener(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection,
    /* dxfg_v4_ipf_connection_state_change_listener_t * */ const JavaObjectHandle<IpfPropertyChangeListener>
        &listener) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_removeStateChangeListener`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_removeStateChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileConnection_removeStateChangeListener,
                       static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()),
                       static_cast<dxfg_v4_ipf_connection_state_change_listener_t *>(listener.get()));
}

bool waitUntilCompleted(
    /* dxfg_v4_ipf_connection_t * */ const JavaObjectHandle<InstrumentProfileConnection> &instrumentProfileConnection,
    std::int64_t timeoutInMs) {
    if (!instrumentProfileConnection) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileConnection_waitUntilCompleted`. The "
            "`instrumentProfileConnection` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileConnection_waitUntilCompleted,
                                        static_cast<dxfg_v4_ipf_connection_t *>(instrumentProfileConnection.get()),
                                        timeoutInMs) == 1;
}

} // namespace IsolatedInstrumentProfileConnection

namespace IsolatedIpfPropertyChangeListener {

/* dxfg_v4_ipf_connection_state_change_listener_t* */ JavaObjectHandle<IpfPropertyChangeListener>
create(/* dxfg_v4_ipf_connection_state_change_listener_func */ void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_IpfPropertyChangeListener_new`. The "
                                       "`userFunc` is nullptr");
    }

    return JavaObjectHandle<IpfPropertyChangeListener>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IpfPropertyChangeListener_new,
        dxfcpp::bit_cast<dxfg_v4_ipf_connection_state_change_listener_func>(userFunc), userData)};
}

} // namespace IsolatedIpfPropertyChangeListener

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE
