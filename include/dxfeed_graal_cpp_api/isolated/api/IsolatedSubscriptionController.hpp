// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXFeed.hpp"
#include "../../api/SubscriptionController.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedSubscriptionController {

// int32_t dxfg_SubscriptionController_attach(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, dxfg_feed_t *feed);
void attach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_SubscriptionController_detach(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, dxfg_feed_t *feed);
void detach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_SubscriptionController_getEventsBatchLimit(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, DXFG_OUT int32_t *eventsBatchLimit);
std::int32_t getEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller);

// int32_t dxfg_SubscriptionController_setEventsBatchLimit(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, int32_t eventsBatchLimit);
void setEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller, std::int32_t eventsBatchLimit);

// int32_t dxfg_SubscriptionController_getAggregationPeriod(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, DXFG_OUT dxfg_time_period_t **aggregationPeriod);
// Returns 0 when the period is not set.
std::int64_t getAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller);

// int32_t dxfg_SubscriptionController_setAggregationPeriodMillis(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, int64_t aggregationPeriod);
void setAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller, std::int64_t aggregationPeriod);

// int32_t dxfg_SubscriptionController_setExecutor(graal_isolatethread_t *thread,
// dxfg_subscription_controller_t *subscriptionController, dxfg_executor_t *executor);
void setExecutor(const JavaObjectHandle<SubscriptionController> &controller, /* dxfg_executor_t* */ void *executor);

} // namespace isolated::api::IsolatedSubscriptionController

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
