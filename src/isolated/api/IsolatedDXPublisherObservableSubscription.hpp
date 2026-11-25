// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp"

#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisherObservableSubscription {
/**
 * Calls the Graal SDK function `dxfg_v4_ObservableSubscription_isClosed` in isolation.
 *
 * @param sub The subscription's handle.
 * @return Returns `true` if this subscription is closed.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool isClosed(const JavaObjectHandle<DXPublisherObservableSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_ObservableSubscription_getEventTypes` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The event types set.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::unordered_set<EventTypeEnum> getEventTypes(const JavaObjectHandle<DXPublisherObservableSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_ObservableSubscription_containsEventType` in isolation.
 *
 * @param sub The subscription's handle.
 * @param eventType The type of event that is checked.
 * @return `true` if this subscription contains the corresponding event type.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool containsEventType(const JavaObjectHandle<DXPublisherObservableSubscription> &sub, const EventTypeEnum &eventType);

/**
 * Calls the Graal SDK function `dxfg_v4_ObservableSubscription_addChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's or ObservableSubscriptionChangeListener's
 * handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void addChangeListener(const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
                       const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener);

/**
 * Calls the Graal SDK function `dxfg_v4_ObservableSubscription_removeChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXPublisherObservableSubscription's or ObservableSubscriptionChangeListener's
 * handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void removeChangeListener(const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
                          const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener);
} // namespace isolated::api::IsolatedDXPublisherObservableSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()