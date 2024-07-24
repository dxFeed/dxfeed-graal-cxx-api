// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <unordered_set>

#include "../../api/DXPublisherObservableSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisherObservableSubscription {
/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_isClosed` in isolation.
 *
 * @param sub The subscription's handle.
 * @return Returns `true` if this subscription is closed.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */
isClosed(/* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_getEventTypes` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The event types set.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::unordered_set<EventTypeEnum> /* dxfg_event_clazz_list_t* */ getEventTypes(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_containsEventType` in isolation.
 *
 * @param sub The subscription's handle.
 * @param eventType The type of event that is checked.
 * @return `true` if this subscription contains the corresponding event type.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ containsEventType(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_event_clazz_t */ const EventTypeEnum & eventType);

/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_addChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_removeChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);
} // namespace isolated::api::IsolatedDXPublisherObservableSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()