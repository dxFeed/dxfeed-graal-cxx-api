// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../internal/Conf.hpp"

#include "../../../ipf/live/InstrumentProfileCollector.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::live {

namespace IsolatedInstrumentProfileCollector {

/* dxfg_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create();
std::int64_t getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
bool updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_instrument_profile_t * */ const JavaObjectHandle<dxfcpp::InstrumentProfile> &ip);
/* dxfg_iterable_ip_t* */ void *
view(/* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
bool addUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener);
bool removeUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener> &listener);

} // namespace IsolatedInstrumentProfileCollector

namespace IsolatedInstrumentProfileUpdateListener {

/* dxfg_ipf_update_listener_t* */ JavaObjectHandle<dxfcpp::InstrumentProfileUpdateListener>
create(/* dxfg_ipf_update_listener_function */ void *userFunc, void *userData);

}

namespace IsolatedInstrumentProfileIterator {

bool hasNext(/* dxfg_iterable_ip_t * */ void *iterable);

/* dxfg_instrument_profile_t* */ void *next(/* dxfg_iterable_ip_t * */ void *iterable);

} // namespace IsolatedInstrumentProfileIterator

} // namespace isolated::ipf::live

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()