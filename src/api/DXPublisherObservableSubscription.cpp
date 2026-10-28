// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
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

            runRelease("DXPublisherObservableSubscription", [&] {
                isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(
                    handle_, registration.bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));
            });
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

// The native functions are called without listenersMutex_: the SDK may call the bridge (releaseChangeListenerAfterClose
// takes the lock) on another thread while it holds its own locks.
std::size_t
DXPublisherObservableSubscription::addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) {
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

    {
        std::lock_guard guard{listenersMutex_};

        listeners_.emplace(id, ListenerRegistration{listener, bridge});
    }

    try {
        isolated::api::IsolatedDXPublisherObservableSubscription::addChangeListener(
            handle_, bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));
    } catch (...) {
        std::lock_guard guard{listenersMutex_};

        listeners_.erase(id);

        throw;
    }

    const auto closed = isClosed();
    std::lock_guard guard{listenersMutex_};

    // Closed while the listener was being added: the SDK may have called the bridge already, which removed it.
    if (closed || !listeners_.contains(id)) {
        listeners_.erase(id);

        return FAKE_LISTENER_ID;
    }

    return id;
}

void DXPublisherObservableSubscription::removeChangeListener(std::size_t changeListenerId) {
    if (changeListenerId == FAKE_LISTENER_ID) {
        return;
    }

    ListenerRegistration registration{};

    {
        std::lock_guard guard{listenersMutex_};

        const auto found = listeners_.find(changeListenerId);

        if (found == listeners_.end()) {
            return;
        }

        registration = std::move(found->second);
        listeners_.erase(found);
    }

    try {
        isolated::api::IsolatedDXPublisherObservableSubscription::removeChangeListener(
            handle_, registration.bridge->getHandle(ObservableSubscriptionChangeListener::Key{}));
    } catch (...) {
        // The listener stays registered in the SDK: keep it, as before the call.
        std::lock_guard guard{listenersMutex_};

        listeners_.emplace(changeListenerId, std::move(registration));

        throw;
    }
}

DXFCPP_END_NAMESPACE
