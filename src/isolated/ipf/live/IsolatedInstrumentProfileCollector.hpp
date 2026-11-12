// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../../include/dxfeed_graal_cpp_api/ipf/live/InstrumentProfileCollector.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live {

namespace IsolatedInstrumentProfileCollector {

/* dxfg_v4_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create();
std::int64_t getLastUpdateTime(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
void updateInstrumentProfile(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_instrument_profile_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &ip);
/* dxfg_v4_iterable_ip_t* */ void *
view(/* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
void addUpdateListener(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener);
void removeUpdateListener(
    /* dxfg_v4_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_v4_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener);

} // namespace IsolatedInstrumentProfileCollector

namespace IsolatedInstrumentProfileUpdateListener {

/* dxfg_v4_ipf_update_listener_t* */ JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener>
create(/* dxfg_v4_ipf_update_listener_function */ void *userFunc, void *userData);
}

namespace IsolatedInstrumentProfileIterator {

bool hasNext(/* dxfg_v4_iterable_ip_t * */ void *iterable);

/* dxfg_v4_instrument_profile_t* */ void *next(/* dxfg_v4_iterable_ip_t * */ void *iterable);

} // namespace IsolatedInstrumentProfileIterator

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()