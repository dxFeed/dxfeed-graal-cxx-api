// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp>
#include <dxfeed_graal_cpp_api/internal/context/ApiContext.hpp>
#include <dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXPublisherObservableSubscription.hpp>
#include <memory>

DXFCPP_BEGIN_NAMESPACE

DXPublisherObservableSubscription::DXPublisherObservableSubscription(
    LockExternalConstructionTag, JavaObjectHandle<DXPublisherObservableSubscription> &&handle)
    : handle_{std::move(handle)} {
}

DXPublisherObservableSubscription::~DXPublisherObservableSubscription() = default;

std::shared_ptr<DXPublisherObservableSubscription>
DXPublisherObservableSubscription::create(JavaObjectHandle<DXPublisherObservableSubscription> &&handle) {
    auto sub = DXPublisherObservableSubscription::createShared(std::move(handle));

    ApiContext::getInstance()->getManager<EntityManager<DXPublisherObservableSubscription>>()->registerEntity(sub);

    return sub;
}

bool DXPublisherObservableSubscription::isClosed() {
    return isolated::api::IsolatedDXPublisherObservableSubscription::isClosed(handle_);
}

std::unordered_set<EventTypeEnum> DXPublisherObservableSubscription::getEventTypes() {
    return isolated::api::IsolatedDXPublisherObservableSubscription::getEventTypes(handle_);
}

bool DXPublisherObservableSubscription::containsEventType(const EventTypeEnum &eventType) {
    return isolated::api::IsolatedDXPublisherObservableSubscription::containsEventType(handle_, eventType);
}

std::size_t
DXPublisherObservableSubscription::addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) {
    isolated::api::IsolatedDXPublisherObservableSubscription::addChangeListener(
        handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

    std::lock_guard guard{listenersMutex_};

    if (lastListenerId_ >= FAKE_LISTENER_ID - 1) {
        return FAKE_LISTENER_ID;
    }

    auto id = ++lastListenerId_;

    listeners_.emplace(id, listener);

    return id;
}

void DXPublisherObservableSubscription::removeChangeListener(std::size_t changeListenerId) {
    std::lock_guard guard{listenersMutex_};

    if (changeListenerId == FAKE_LISTENER_ID) {
        return;
    }

    if (const auto found = listeners_.find(changeListenerId); found != listeners_.end()) {
        const auto listener = found->second;

        isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(
            handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

        listeners_.erase(found);
    }
}

DXFCPP_END_NAMESPACE