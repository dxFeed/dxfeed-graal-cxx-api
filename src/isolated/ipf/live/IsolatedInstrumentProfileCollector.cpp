// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/ipf/live/IsolatedInstrumentProfileCollector.hpp>

DXFCPP_BEGIN_NAMESPACE

//TODO: implement

namespace isolated::ipf::live::IsolatedInstrumentProfileCollector {
/* dxfg_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create() {
    return JavaObjectHandle<InstrumentProfileCollector>{nullptr};
}

std::int64_t getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    return {};
}

bool updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_instrument_profile_t * */ void *ip) {
    return {};
}

/* dxfg_iterable_ip_t* */ void *
view(/* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector) {
    return nullptr;
}

bool addUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener) {
    return {};
}

bool removeUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener) {
    return {};
}

} // namespace isolated::ipf::live::IsolatedInstrumentProfileCollector

DXFCPP_END_NAMESPACE