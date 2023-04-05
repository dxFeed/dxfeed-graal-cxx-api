// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <memory>
#include <utility>
#include <string>

namespace dxfcpp {

std::shared_ptr<DXFeed> DXFeed::getInstance() noexcept {
    if constexpr (isDebug) {
        debug("DXFeed::getInstance()");
    }

    return DXEndpoint::getInstance()->getFeed();
}

void DXFeed::attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (isDebug) {
        debug("{}::attachSubscription({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_attachSubscription(threadHandle, handler, subHandler) == 0;
            },
            false)) {

        subscriptions_.emplace(subscription);
    }
}

void DXFeed::detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (isDebug) {
        debug("{}::detachSubscription({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscription(threadHandle, handler, subHandler) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

void DXFeed::detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (isDebug) {
        debug("{}::detachSubscriptionAndClear({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscriptionAndClear(threadHandle, handler, subHandler) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(const EventTypeEnum &eventType) noexcept {
    if constexpr (isDebug) {
        debug("{}::createSubscription(eventType = {})", toString(), eventType.getName());
    }

    auto sub = DXFeedSubscription::create(eventType);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription>
DXFeed::createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
    if constexpr (isDebug) {
        debug("{}::createSubscription(eventTypes = {})", toString(),
              namesToString(eventTypes.begin(), eventTypes.end()));
    }

    auto sub = DXFeedSubscription::create(eventTypes);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeed> DXFeed::create(void *feedHandle) noexcept {
    std::shared_ptr<DXFeed> feed{new (std::nothrow) DXFeed{}};

    feed->handler_ = handler_utils::JavaObjectHandler<DXFeed>(feedHandle);

    return feed;
}

}