// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/DXPublisher.hpp"

DXFCPP_BEGIN_NAMESPACE

struct DXPublisherObservableSubscription;

namespace isolated::api::IsolatedDXPublisher {

/**
 * Calls the Graal SDK function `dxfg_v4_DXPublisher_publishEvents` in isolation.
 *
 * @param publisher The publisher's handle.
 * @param events Events to be published.
 * @throws InvalidArgumentException if publisher handle is invalid or events' pointer is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void publishEvents(const JavaObjectHandle<dxfcpp::DXPublisher> &publisher, void *events);

/**
 * Calls the Graal SDK function `dxfg_v4_DXPublisher_getSubscription` in isolation.
 *
 * @param publisher The publisher's handle.
 * @param eventType The event type.
 * @throws InvalidArgumentException if publisher handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 * @return The underlying observable subscription's handle.
 */
JavaObjectHandle<DXPublisherObservableSubscription>
getSubscription(const JavaObjectHandle<dxfcpp::DXPublisher> &publisher, const EventTypeEnum &eventType);

} // namespace isolated::api::IsolatedDXPublisher

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()