// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../include/dxfeed_graal_c_api/api.h"
#include "../include/dxfeed_graal_cpp_api/api.hpp"
#include "../include/dxfeed_graal_cpp_api/entity/SharedEntity.hpp"
#include "../include/dxfeed_graal_cpp_api/event/EventType.hpp"

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

auto C = ApiContext::getInstance();
auto MM = ApiContext::getInstance()->getManager<MetricsManager>();

SharedEntity::SharedEntity() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", 1);
#endif
}

SharedEntity::~SharedEntity() noexcept {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", -1);
#endif
}

EventType::EventType() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", 1);
#endif
}

EventType::~EventType() noexcept {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", -1);
#endif
}

std::int64_t EventType::getEventTime() const noexcept {
    return 0;
}

void EventType::setEventTime(std::int64_t eventTime) noexcept {
    ignoreUnused(eventTime);
    // The default implementation is empty
}

void EventType::assign(std::shared_ptr<EventType> event) {
    ignoreUnused(event);
}

std::string EventType::toString() const {
    return "EventType{}";
}

DXFCPP_END_NAMESPACE
