// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../../include/dxfeed_graal_cpp_api/api/SubscriptionController.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedSubscriptionController {

/// dxfg_v4_SubscriptionController_attach
void attach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed);

/// dxfg_v4_SubscriptionController_detach
void detach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed);

/// dxfg_v4_SubscriptionController_getEventsBatchLimit
std::int32_t getEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller);

/// dxfg_v4_SubscriptionController_setEventsBatchLimit
void setEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller, std::int32_t eventsBatchLimit);

/// dxfg_v4_SubscriptionController_getAggregationPeriod
// Returns 0 when the period is not set.
std::int64_t getAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller);

/// dxfg_v4_SubscriptionController_setAggregationPeriodMillis
void setAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller, std::int64_t aggregationPeriod);

/// dxfg_v4_SubscriptionController_setExecutor
void setExecutor(const JavaObjectHandle<SubscriptionController> &controller, /* dxfg_v4_executor_t* */ void *executor);

} // namespace isolated::api::IsolatedSubscriptionController

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
