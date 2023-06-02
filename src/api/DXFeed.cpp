// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

std::shared_ptr<DXFeed> DXFeed::getInstance() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeed::getInstance()");
    }

    return DXEndpoint::getInstance()->getFeed();
}

void DXFeed::attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::attachSubscription(" + subscription->toString() + ")");
    }

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_attachSubscription(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                      subHandler) == 0;
            },
            false)) {

        subscriptions_.emplace(subscription);
    }
}

void DXFeed::detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscription(" + subscription->toString() + ")");
    }

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscription(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                      subHandler) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

void DXFeed::detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscriptionAndClear(" + subscription->toString() + ")");
    }

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (runIsolatedOrElse(
            [handler = bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscriptionAndClear(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                              handler, subHandler) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(const EventTypeEnum &eventType) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::createSubscription(eventType = " + eventType.getName() + ")");
    }

    auto sub = DXFeedSubscription::create(eventType);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription>
DXFeed::createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::createSubscription(eventTypes = " +
                        namesToString(eventTypes.begin(), eventTypes.end()) + ")");
    }

    auto sub = DXFeedSubscription::create(eventTypes);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeed> DXFeed::create(void *feedHandle) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeed::create(" + dxfcpp::toString(feedHandle) + ")");
    }

    std::shared_ptr<DXFeed> feed{new (std::nothrow) DXFeed{}};

    // TODO: error handling

    if (feed) {
        feed->handler_ = JavaObjectHandler<DXFeed>(feedHandle);
    }

    return feed;
}

std::string DXFeed::toString() const noexcept { return fmt::format("DXFeed{{{}}}", handler_.toString()); }

} // namespace dxfcpp