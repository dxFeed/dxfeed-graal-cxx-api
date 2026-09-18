// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXPublisherObservableSubscription.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

DXPublisherObservableSubscription::DXPublisherObservableSubscription(
    LockExternalConstructionTag, JavaObjectHandle<DXPublisherObservableSubscription> &&handle)
    : handle_{std::move(handle)} {
}

DXPublisherObservableSubscription::~DXPublisherObservableSubscription() noexcept {
    decltype(listeners_) listeners;

    {
        std::lock_guard guard{listenersMutex_};
        listeners.swap(listeners_);
    }

    if (handle_) {
        for (const auto &[id, registration] : listeners) {
            ignoreUnused(id);

            try {
                isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(
                    handle_, registration.bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));
            } catch (...) {
                // Destructors must remain noexcept even if the native isolate is already shutting down.
            }
        }
    }
}

std::shared_ptr<DXPublisherObservableSubscription>
DXPublisherObservableSubscription::create(JavaObjectHandle<DXPublisherObservableSubscription> &&handle) {
    return createShared(std::move(handle));
}

void DXPublisherObservableSubscription::releaseChangeListenerAfterClose(std::size_t listenerId) {
    std::lock_guard guard{listenersMutex_};
    listeners_.erase(listenerId);
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
    std::lock_guard guard{listenersMutex_};

    if (isClosed()) {
        return FAKE_LISTENER_ID;
    }

    if (lastListenerId_ >= FAKE_LISTENER_ID - 1) {
        return FAKE_LISTENER_ID;
    }

    const auto id = ++lastListenerId_;
    const std::weak_ptr<DXPublisherObservableSubscription> weakSelf = sharedAs<DXPublisherObservableSubscription>();
    const std::weak_ptr<ObservableSubscriptionChangeListener> weakListener = listener;
    auto bridge = ObservableSubscriptionChangeListener::create(
        [weakListener](const std::unordered_set<SymbolWrapper> &symbols) {
            if (const auto target = weakListener.lock()) {
                target->notifySymbolsAdded(ObservableSubscriptionChangeListener::Key{}, symbols);
            }
        },
        [weakListener](const std::unordered_set<SymbolWrapper> &symbols) {
            if (const auto target = weakListener.lock()) {
                target->notifySymbolsRemoved(ObservableSubscriptionChangeListener::Key{}, symbols);
            }
        },
        [weakSelf, weakListener, id] {
            if (const auto target = weakListener.lock()) {
                target->notifySubscriptionClosed(ObservableSubscriptionChangeListener::Key{});
            }

            if (const auto self = weakSelf.lock()) {
                self->releaseChangeListenerAfterClose(id);
            }
        });

    listeners_.emplace(id, ListenerRegistration{listener, bridge});

    try {
        isolated::api::IsolatedDXPublisherObservableSubscription::addChangeListener(
            handle_, bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));
    } catch (...) {
        listeners_.erase(id);
        throw;
    }

    if (isClosed() || !listeners_.contains(id)) {
        listeners_.erase(id);
        return FAKE_LISTENER_ID;
    }

    return id;
}

void DXPublisherObservableSubscription::removeChangeListener(std::size_t changeListenerId) {
    std::lock_guard guard{listenersMutex_};

    if (changeListenerId == FAKE_LISTENER_ID) {
        return;
    }

    if (const auto found = listeners_.find(changeListenerId); found != listeners_.end()) {
        const auto bridge = found->second.bridge;

        isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(
            handle_, bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));

        listeners_.erase(changeListenerId);
    }
}

DXFCPP_END_NAMESPACE
