// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedSubscriptionController.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/util/IsolatedTimePeriod.hpp"

#include <dxfg_api.h>

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedSubscriptionController {

namespace {

dxfg_subscription_controller_t *toGraal(const JavaObjectHandle<SubscriptionController> &controller,
                                        const char *function) {
    if (!controller) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `controller` handle is invalid");
    }

    return static_cast<dxfg_subscription_controller_t *>(controller.get());
}

dxfg_feed_t *toGraal(const JavaObjectHandle<DXFeed> &feed, const char *function) {
    if (!feed) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `feed` handle is invalid");
    }

    return static_cast<dxfg_feed_t *>(feed.get());
}

} // namespace

void attach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed) {
    constexpr auto function = "dxfg_SubscriptionController_attach";

    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_attach, toGraal(controller, function),
                                           toGraal(feed, function));
}

void detach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed) {
    constexpr auto function = "dxfg_SubscriptionController_detach";

    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_detach, toGraal(controller, function),
                                           toGraal(feed, function));
}

std::int32_t getEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller) {
    std::int32_t eventsBatchLimit{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_getEventsBatchLimit,
                                           toGraal(controller, "dxfg_SubscriptionController_getEventsBatchLimit"),
                                           &eventsBatchLimit);

    return eventsBatchLimit;
}

void setEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller, std::int32_t eventsBatchLimit) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_setEventsBatchLimit,
                                           toGraal(controller, "dxfg_SubscriptionController_setEventsBatchLimit"),
                                           eventsBatchLimit);
}

std::int64_t getAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller) {
    // Not the millis variant: it fails while the period is not set (null in Java).
    dxfg_time_period_t *aggregationPeriod{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_getAggregationPeriod,
                                           toGraal(controller, "dxfg_SubscriptionController_getAggregationPeriod"),
                                           &aggregationPeriod);

    if (!aggregationPeriod) {
        // Java treats a period that is not set as zero (no aggregation beyond the feed's own).
        return 0;
    }

    return isolated::util::IsolatedTimePeriod::getTime(JavaObjectHandle<TimePeriod>{aggregationPeriod});
}

void setAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller, std::int64_t aggregationPeriod) {
    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_SubscriptionController_setAggregationPeriodMillis,
        toGraal(controller, "dxfg_SubscriptionController_setAggregationPeriodMillis"), aggregationPeriod);
}

void setExecutor(const JavaObjectHandle<SubscriptionController> &controller, void *executor) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_SubscriptionController_setExecutor,
                                           toGraal(controller, "dxfg_SubscriptionController_setExecutor"),
                                           static_cast<dxfg_executor_t *>(executor));
}

} // namespace isolated::api::IsolatedSubscriptionController

DXFCPP_END_NAMESPACE
