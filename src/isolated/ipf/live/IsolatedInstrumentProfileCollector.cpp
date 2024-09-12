// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/ipf/live/IsolatedInstrumentProfileCollector.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live::IsolatedInstrumentProfileCollector {
/* dxfg_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create() {
    return JavaObjectHandle<InstrumentProfileCollector>{
        runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfileCollector_new)};
}

std::int64_t getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileCollector_getLastUpdateTime`. The "
            "`instrumentProfileCollector` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileCollector_getLastUpdateTime,
                                              static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get()));
}

bool updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_instrument_profile_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &ip) {
    if (!instrumentProfileCollector || !ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileCollector_updateInstrumentProfile`. The "
            "`instrumentProfileCollector` handle or `ip` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileCollector_updateInstrumentProfile,
                                              static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get()),
                                              static_cast<dxfg_instrument_profile_t *>(ip.get())) == 0;
}

/* dxfg_iterable_ip_t* */ void *
view(/* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    if (!instrumentProfileCollector) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfileCollector_view`. The "
                                       "`instrumentProfileCollector` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfileCollector_view, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get())));
}

bool addUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener) {
    if (!instrumentProfileCollector || !listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileCollector_addUpdateListener`. The "
            "`instrumentProfileCollector` handle or `listener` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileCollector_addUpdateListener,
                                              static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get()),
                                              static_cast<dxfg_ipf_update_listener_t *>(listener.get())) == 0;
}

bool removeUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener) {
    if (!instrumentProfileCollector || !listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileCollector_removeUpdateListener`. The "
            "`instrumentProfileCollector` handle or `listener` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfileCollector_removeUpdateListener,
                                              static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollector.get()),
                                              static_cast<dxfg_ipf_update_listener_t *>(listener.get())) == 0;
}

} // namespace isolated::ipf::live::IsolatedInstrumentProfileCollector

DXFCPP_END_NAMESPACE