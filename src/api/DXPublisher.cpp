// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<DXPublisher> DXPublisher::getInstance() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::getInstance()");
    }

    return DXEndpoint::getInstance()->getPublisher();
}

std::shared_ptr<DXPublisher> DXPublisher::create(void *handle) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::create(" + dxfcpp::toString(handle) + ")");
    }

    std::shared_ptr<DXPublisher> publisher{new DXPublisher{}};

    auto id = ApiContext::getInstance()->getManager<DXPublisherManager>()->registerEntity(publisher);
    ignore_unused(id);

    publisher->handle_ = JavaObjectHandle<DXPublisher>(handle);

    return publisher;
}

std::shared_ptr<ObservableSubscription> DXPublisher::getSubscription(const EventTypeEnum &eventType) {
    std::lock_guard guard{mutex_};

    if (subscription_) {
        return subscription_;
    }

    subscription_ = DXPublisherObservableSubscription::create(isolated::api::IsolatedDXPublisher::getSubscription(handle_, eventType));

    return subscription_;
}

std::string DXPublisher::toString() const noexcept {
    return fmt::format("DXPublisher{{{}}}", handle_.toString());
}

void DXPublisher::publishEventsImpl(void *graalEventsList) const noexcept {
    std::lock_guard guard{mutex_};

    isolated::api::IsolatedDXPublisher::publishEvents(handle_, graalEventsList);
}

DXFCPP_END_NAMESPACE