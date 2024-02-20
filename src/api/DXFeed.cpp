// Copyright (c) 2024 Devexperts LLC.
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

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [](auto threadHandle, auto &&...params) {
                return dxfg_DXFeed_attachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
                       0;
            },
            false, static_cast<dxfg_feed_t *>(handle_.get()),
            static_cast<dxfg_subscription_t *>(subscription->handle_.get()))) {

        subscriptions_.emplace(subscription);
    }
}

void DXFeed::detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscription(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [](auto threadHandle, auto &&...params) {
                return dxfg_DXFeed_detachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
                       0;
            },
            false, static_cast<dxfg_feed_t *>(handle_.get()),
            static_cast<dxfg_subscription_t *>(subscription->handle_.get()))) {

        subscriptions_.erase(subscription);
    }
}

void DXFeed::detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscriptionAndClear(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [](auto threadHandle, auto &&...params) {
                return dxfg_DXFeed_detachSubscriptionAndClear(static_cast<graal_isolatethread_t *>(threadHandle),
                                                              params...) == 0;
            },
            false, static_cast<dxfg_feed_t *>(handle_.get()),
            static_cast<dxfg_subscription_t *>(subscription->handle_.get()))) {

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

    auto id = ApiContext::getInstance()->getManager<DXFeedManager>()->registerEntity(feed);
    ignore_unused(id);

    // TODO: error handling [EN-8232]

    if (feed) {
        feed->handle_ = JavaObjectHandle<DXFeed>(feedHandle);
    }

    return feed;
}

std::string DXFeed::toString() const noexcept {
    return fmt::format("DXFeed{{{}}}", handle_.toString());
}

} // namespace dxfcpp