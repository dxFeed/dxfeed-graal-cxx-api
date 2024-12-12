// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXPublisher.hpp"

DXFCPP_BEGIN_NAMESPACE

struct DXPublisherObservableSubscription;

namespace isolated::api::IsolatedDXPublisher {

/**
 * Calls the Graal SDK function `dxfg_DXPublisher_publishEvents` in isolation.
 *
 * @param publisher The publisher's handle.
 * @param events Events to be published.
 * @throws InvalidArgumentException if publisher handle is invalid or events' pointer is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ publishEvents(/* dxfg_publisher_t * */ const JavaObjectHandle<dxfcpp::DXPublisher> &publisher,
                                 /* dxfg_event_type_list * */ void *events);

/**
 * Calls the Graal SDK function `dxfg_DXPublisher_getSubscription` in isolation.
 *
 * @param publisher The publisher's handle.
 * @param eventType The event type.
 * @throws InvalidArgumentException if publisher handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 * @return The underlying observable subscription's handle.
 */
JavaObjectHandle<DXPublisherObservableSubscription> /* dxfg_observable_subscription_t * */
getSubscription(/* dxfg_publisher_t * */ const JavaObjectHandle<dxfcpp::DXPublisher> &publisher,
                                 /* dxfg_event_clazz_t */ const EventTypeEnum &eventType);

} // namespace isolated::api::IsolatedDXPublisher

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()