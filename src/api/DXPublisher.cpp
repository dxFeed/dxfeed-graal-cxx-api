// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api/DXEndpoint.hpp>
#include <dxfeed_graal_cpp_api/api/DXPublisher.hpp>
#include <dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp>

#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXPublisher.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<DXPublisher> DXPublisher::getInstance() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXPublisher::getInstance()");
    }

    return DXEndpoint::getInstance()->getPublisher();
}

std::shared_ptr<DXPublisher> DXPublisher::create(void *handle) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXPublisher::create(" + dxfcpp::toString(handle) + ")");
    }

    std::shared_ptr<DXPublisher> publisher{new DXPublisher{}};

    auto id = ApiContext::getInstance()->getManager<EntityManager<DXPublisher>>()->registerEntity(publisher);
    ignoreUnused(id);

    publisher->handle_ = JavaObjectHandle<DXPublisher>(handle);

    return publisher;
}

std::shared_ptr<ObservableSubscription> DXPublisher::getSubscription(const EventTypeEnum &eventType) {
    std::lock_guard guard{mutex_};

    if (subscriptions_.contains(eventType)) {
        return subscriptions_.at(eventType);
    }

    auto sub = DXPublisherObservableSubscription::create(
        isolated::api::IsolatedDXPublisher::getSubscription(handle_, eventType));

    subscriptions_.emplace(std::cref(eventType), sub);

    return sub;
}

std::string DXPublisher::toString() const {
    return std::string("DXPublisher{") + handle_.toString() + "}";
}

void DXPublisher::publishEventsImpl(void *graalEventsList) const noexcept {
    std::lock_guard guard{mutex_};

    isolated::api::IsolatedDXPublisher::publishEvents(handle_, graalEventsList);
}

DXPublisher::DXPublisher() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXPublisher()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXPublisher", 1);
#endif
}

DXPublisher::~DXPublisher() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXPublisher{" + handle_.toString() + "}::~DXPublisher()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXPublisher", -1);
#endif
}

DXFCPP_END_NAMESPACE