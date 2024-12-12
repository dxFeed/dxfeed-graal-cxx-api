// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/IndexedEventSource.hpp"
#include "../../symbols/SymbolWrapper.hpp"
#include "../../event/EventTypeEnum.hpp"
#include "ObservableSubscriptionChangeListener.hpp"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * Observable set of subscription symbols.
 */
struct DXFCPP_EXPORT ObservableSubscription {
    virtual ~ObservableSubscription() = default;

    /**
     * @return `true` if this subscription is closed.
     * The observable subscription that is returned as a result of DXPublisher::getSubscription() is closed when the
     * corresponding DXEndpoint is closed.
     */
    virtual bool isClosed() = 0;

    /**
     * @return a set of event types for this subscription. The resulting set cannot be modified.
     * The observable subscription that is returned as a result of DXPublisher::getSubscription() has a singleton set
     * of event types.
     */
    virtual std::unordered_set<EventTypeEnum> getEventTypes() = 0;

    /**
     * Returns `true` if this subscription contains the corresponding event type.
     *
     * @param eventType The type of event that is checked.
     * @return `true` if this subscription contains the corresponding event type.
     * @see #getEventTypes()
     */
    virtual bool containsEventType(const EventTypeEnum &eventType) = 0;

    /**
     * Adds subscription change listener. This method does nothing if subscription is closed.
     * Otherwise, it installs the corresponding listener and immediately
     * invokes ObservableSubscriptionChangeListener::symbolsAdded() on the given listener while holding the lock for
     * this subscription. This way the given listener synchronously receives existing subscription state and is
     * synchronously notified on all changes in subscription afterwards.
     *
     * @param listener The subscription change listener.
     * @return The listener id
     */
    virtual std::size_t addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) = 0;

    /**
     * Removes subscription change listener by id. This method does nothing if the listener with the given id was not
     * installed or was already removed as subscription change listener for this subscription. Otherwise, it removes the
     * corresponding listener and immediately invokes ObservableSubscriptionChangeListener::subscriptionClosed() on the
     * given listener while holding the lock for this subscription.
     *
     * @param id The subscription change listener id.
     */
    virtual void removeChangeListener(std::size_t id) = 0;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()