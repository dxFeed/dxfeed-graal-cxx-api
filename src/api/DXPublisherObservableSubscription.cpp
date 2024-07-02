// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

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
    isolated::api::IsolatedDXPublisherObservableSubscription::addChangeListener(handle_, listener->getHandle());

    std::lock_guard guard{listenersMutex_};

    if (lastListenerId_ >= FAKE_LISTENER_ID - 1) {
        return FAKE_LISTENER_ID;
    }

    auto id = ++lastListenerId_;

    listeners_.emplace(id, listener);

    return id;
}

void DXPublisherObservableSubscription::removeChangeListener(std::size_t id) {
    std::lock_guard guard{listenersMutex_};

    if (id == FAKE_LISTENER_ID) {
        return;
    }

    if (auto found = listeners_.find(id); found != listeners_.end()) {
        auto listener = found->second;

        isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(handle_, listener->getHandle());

        listeners_.erase(found);
    }
}

DXFCPP_END_NAMESPACE