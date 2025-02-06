// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/entity/SharedEntity.hpp"
#include "dxfeed_graal_cpp_api/event/EventType.hpp"
#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <utility>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

auto C = ApiContext::getInstance();
auto MM = ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>();

SharedEntity::SharedEntity() {
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", 1);
}

SharedEntity::~SharedEntity() noexcept {
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", -1);
}

EventType::EventType() {
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", 1);
}

EventType::~EventType() noexcept {
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", -1);
}

DXFCPP_END_NAMESPACE
