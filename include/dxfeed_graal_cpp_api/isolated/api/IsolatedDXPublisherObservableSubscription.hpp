// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <unordered_set>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXPublisherObservableSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisherObservableSubscription {
/**
 * Calls the Graal SDK function `dxfg_ObservableSubscription_isClosed` in isolation.
 *
 * @param sub The subscription's handle.
 * @return Returns `true` if this subscription is closed.
 * @throws std::invalid_argument if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */
isClosed(/* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub);


std::unordered_set<EventTypeEnum> /* dxfg_event_clazz_list_t* */ getEventTypes(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub);
bool /* int32_t */ containsEventType(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_event_clazz_t */ EventTypeEnum eventType);
void /* int32_t */ addChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);
void /* int32_t */ removeChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);
} // namespace isolated::api::IsolatedDXPublisherObservableSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()