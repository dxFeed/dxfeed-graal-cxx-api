// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <typeinfo>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

bool DXPublisherObservableSubscription::isClosed() {
    return false;
}

std::unordered_set<EventTypeEnum> DXPublisherObservableSubscription::getEventTypes() {
    return {};
}

bool DXPublisherObservableSubscription::containsEventType(const EventTypeEnum &eventType) {
    return false;
}

std::size_t
DXPublisherObservableSubscription::addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) {
    return Id<ObservableSubscriptionChangeListener>::UNKNOWN.getValue();
}

void DXPublisherObservableSubscription::removeChangeListener(std::size_t id) {
}

DXFCPP_END_NAMESPACE