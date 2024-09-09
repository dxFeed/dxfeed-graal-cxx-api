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

namespace isolated::ipf::live::IsolatedInstrumentProfileCollector {
/* dxfg_ipf_collector_t* */ JavaObjectHandle<InstrumentProfileCollector> create();
std::int64_t getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
bool updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_instrument_profile_t * */ void *ip);
/* dxfg_iterable_ip_t* */ void *
view(/* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector);
bool addUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener);
bool removeUpdateListener(
    /* dxfg_ipf_collector_t* */ const JavaObjectHandle<InstrumentProfileCollector> &instrumentProfileCollector,
    /* dxfg_ipf_update_listener_t* */ const JavaObjectHandle<InstrumentProfileUpdateListener> &listener);
} // namespace isolated::ipf::live::IsolatedInstrumentProfileCollector

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()