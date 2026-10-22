// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/SubscriptionController.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedSubscriptionController.hpp"

#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace {

const std::shared_ptr<DXFeed> &checkFeed(const std::shared_ptr<DXFeed> &feed) {
    if (!feed) {
        throw InvalidArgumentException("The `feed` is nullptr");
    }

    return feed;
}

} // namespace

SubscriptionController::SubscriptionController(LockExternalConstructionTag,
                                               JavaObjectHandle<SubscriptionController> &&handle)
    : handle_(std::move(handle)) {
}

SubscriptionController::~SubscriptionController() noexcept = default;

void SubscriptionController::attach(const std::shared_ptr<DXFeed> &feed) const {
    isolated::api::IsolatedSubscriptionController::attach(handle_, checkFeed(feed)->handle_);
}

void SubscriptionController::detach(const std::shared_ptr<DXFeed> &feed) const {
    isolated::api::IsolatedSubscriptionController::detach(handle_, checkFeed(feed)->handle_);
}

std::int32_t SubscriptionController::getEventsBatchLimit() const {
    return isolated::api::IsolatedSubscriptionController::getEventsBatchLimit(handle_);
}

void SubscriptionController::setEventsBatchLimit(std::int32_t eventsBatchLimit) const {
    isolated::api::IsolatedSubscriptionController::setEventsBatchLimit(handle_, eventsBatchLimit);
}

TimePeriod SubscriptionController::getAggregationPeriod() const {
    return TimePeriod::valueOf(isolated::api::IsolatedSubscriptionController::getAggregationPeriod(handle_));
}

void SubscriptionController::setAggregationPeriod(const TimePeriod &aggregationPeriod) const {
    setAggregationPeriod(aggregationPeriod.getTime());
}

void SubscriptionController::setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) const {
    setAggregationPeriod(static_cast<std::int64_t>(aggregationPeriod.count()));
}

void SubscriptionController::setAggregationPeriod(std::int64_t aggregationPeriod) const {
    isolated::api::IsolatedSubscriptionController::setAggregationPeriod(handle_, aggregationPeriod);
}

void SubscriptionController::setExecutor(std::nullptr_t) const {
    setExecutorImpl(nullptr);
}

void SubscriptionController::setExecutorImpl(void *executor) const {
    isolated::api::IsolatedSubscriptionController::setExecutor(handle_, executor);
}

DXFCPP_END_NAMESPACE
