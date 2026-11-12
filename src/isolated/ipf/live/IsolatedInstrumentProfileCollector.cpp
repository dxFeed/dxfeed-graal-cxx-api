// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedInstrumentProfileCollector.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live {

namespace IsolatedInstrumentProfileCollector {
/* dxfg_v4_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create() {
    return JavaObjectHandle<InstrumentProfileCollector>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_InstrumentProfileCollector_new)};
}

std::int64_t getLastUpdateTime(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_getLastUpdateTime`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileCollector_getLastUpdateTime,
                                        static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get()));
}

void updateInstrumentProfile(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_instrument_profile_t * */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_updateInstrumentProfileHandle`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_updateInstrumentProfileHandle`. The "
            "`ip` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileCollector_updateInstrumentProfileHandle,
                       static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get()),
                       static_cast<dxfg_v4_instrument_profile_t *>(ip.get()));
}

/* dxfg_v4_iterable_ip_t* */ void *
view(/* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_InstrumentProfileCollector_view`. The "
                                       "`instrumentProfileCollector` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_InstrumentProfileCollector_view,
                                            static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get())));
}

void addUpdateListener(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_addUpdateListener`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_addUpdateListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileCollector_addUpdateListener,
                       static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get()),
                       static_cast<dxfg_v4_ipf_update_listener_t *>(listener.get()));
}

void removeUpdateListener(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_removeUpdateListener`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileCollector_removeUpdateListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_InstrumentProfileCollector_removeUpdateListener,
                       static_cast<dxfg_v4_ipf_collector_t *>(instrumentProfileCollector.get()),
                       static_cast<dxfg_v4_ipf_update_listener_t *>(listener.get()));
}

} // namespace IsolatedInstrumentProfileCollector

namespace IsolatedInstrumentProfileUpdateListener {

/* dxfg_v4_ipf_update_listener_t* */ JavaObjectHandle<InstrumentProfileUpdateListener>
create(/* dxfg_v4_ipf_update_listener_function */ void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_InstrumentProfileUpdateListener_new`. The "
                                       "`userFunc` is nullptr");
    }

    return JavaObjectHandle<InstrumentProfileUpdateListener>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileUpdateListener_new, dxfcpp::bit_cast<dxfg_v4_ipf_update_listener_function>(userFunc),
        userData)};
}

} // namespace IsolatedInstrumentProfileUpdateListener

namespace IsolatedInstrumentProfileIterator {

bool hasNext(/* dxfg_v4_iterable_ip_t * */ void *iterable) {
    if (!iterable) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Iterable_InstrumentProfile_hasNext`. The "
                                       "`iterable` is nullptr");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Iterable_InstrumentProfile_hasNext,
                                        static_cast<dxfg_v4_iterable_ip_t *>(iterable)) == 1;
}

/* dxfg_v4_instrument_profile_t* */ void *next(/* dxfg_v4_iterable_ip_t * */ void *iterable) {
    if (!iterable) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Iterable_InstrumentProfile_nextHandle`. "
                                       "The `iterable` is nullptr");
    }

    return runGraalV4FunctionWithNonNullResult(dxfg_v4_Iterable_InstrumentProfile_nextHandle,
                                               static_cast<dxfg_v4_iterable_ip_t *>(iterable));
}

} // namespace IsolatedInstrumentProfileIterator

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE
