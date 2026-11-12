// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedSubscriptionController.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../util/IsolatedTimePeriod.hpp"

#include <dxfg_api.h>

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedSubscriptionController {

// The dxfg_v4_SubscriptionController_* functions are experimental: each call checks the advanced revision of the SDK
// library first.

namespace {

dxfg_v4_subscription_controller_t *toGraal(const JavaObjectHandle<SubscriptionController> &controller,
                                           const char *function) {
    if (!controller) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `controller` handle is invalid");
    }

    return static_cast<dxfg_v4_subscription_controller_t *>(controller.get());
}

dxfg_v4_feed_t *toGraal(const JavaObjectHandle<DXFeed> &feed, const char *function) {
    if (!feed) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `feed` handle is invalid");
    }

    return static_cast<dxfg_v4_feed_t *>(feed.get());
}

} // namespace

/// dxfg_v4_SubscriptionController_attach
void attach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed) {
    constexpr auto function = "dxfg_v4_SubscriptionController_attach";
    auto *graalController = toGraal(controller, function);
    auto *graalFeed = toGraal(feed, function);

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_SubscriptionController_attach, graalController, graalFeed);
}

/// dxfg_v4_SubscriptionController_detach
void detach(const JavaObjectHandle<SubscriptionController> &controller, const JavaObjectHandle<DXFeed> &feed) {
    constexpr auto function = "dxfg_v4_SubscriptionController_detach";
    auto *graalController = toGraal(controller, function);
    auto *graalFeed = toGraal(feed, function);

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_SubscriptionController_detach, graalController, graalFeed);
}

/// dxfg_v4_SubscriptionController_getEventsBatchLimit
std::int32_t getEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller) {
    auto *graalController = toGraal(controller, "dxfg_v4_SubscriptionController_getEventsBatchLimit");

    Isolate::checkAdvancedRevision();

    return runGraalV4FunctionWithResult(dxfg_v4_SubscriptionController_getEventsBatchLimit, graalController);
}

/// dxfg_v4_SubscriptionController_setEventsBatchLimit
void setEventsBatchLimit(const JavaObjectHandle<SubscriptionController> &controller, std::int32_t eventsBatchLimit) {
    auto *graalController = toGraal(controller, "dxfg_v4_SubscriptionController_setEventsBatchLimit");

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_SubscriptionController_setEventsBatchLimit, graalController, eventsBatchLimit);
}

/// dxfg_v4_SubscriptionController_getAggregationPeriod
std::int64_t getAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller) {
    auto *graalController = toGraal(controller, "dxfg_v4_SubscriptionController_getAggregationPeriod");

    Isolate::checkAdvancedRevision();

    // Not the millis variant: it fails while the period is not set (null in Java).
    auto *aggregationPeriod =
        runGraalV4FunctionWithResult(dxfg_v4_SubscriptionController_getAggregationPeriod, graalController);

    if (!aggregationPeriod) {
        // Java treats a period that is not set as zero (no aggregation beyond the feed's own).
        return 0;
    }

    return isolated::util::IsolatedTimePeriod::getTime(JavaObjectHandle<TimePeriod>{aggregationPeriod});
}

/// dxfg_v4_SubscriptionController_setAggregationPeriodMillis
void setAggregationPeriod(const JavaObjectHandle<SubscriptionController> &controller, std::int64_t aggregationPeriod) {
    auto *graalController = toGraal(controller, "dxfg_v4_SubscriptionController_setAggregationPeriodMillis");

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_SubscriptionController_setAggregationPeriodMillis, graalController, aggregationPeriod);
}

/// dxfg_v4_SubscriptionController_setExecutor
void setExecutor(const JavaObjectHandle<SubscriptionController> &controller, void *executor) {
    auto *graalController = toGraal(controller, "dxfg_v4_SubscriptionController_setExecutor");

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_SubscriptionController_setExecutor, graalController,
                       static_cast<dxfg_v4_executor_t *>(executor));
}

} // namespace isolated::api::IsolatedSubscriptionController

DXFCPP_END_NAMESPACE
