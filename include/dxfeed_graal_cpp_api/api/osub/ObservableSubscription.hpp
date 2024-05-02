// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/IndexedEventSource.hpp"
#include "../../symbols/SymbolWrapper.hpp"

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
     * @return `true` if this subscription contains the corresponding event type.
     * @see #getEventTypes()
     */
    virtual bool containsEventType(const EventTypeEnum &eventType) = 0;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()