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

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<DXFeed> DXFeed::getInstance() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeed::getInstance()");
    }

    return DXEndpoint::getInstance()->getFeed();
}

void DXFeed::attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::attachSubscription(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [handle = static_cast<dxfg_feed_t *>(handle_.get()),
             subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
                return dxfg_DXFeed_attachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                      subHandle) == 0;
            },
            false)) {
    }
}

void DXFeed::detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscription(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [handle = static_cast<dxfg_feed_t *>(handle_.get()),
             subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                      subHandle) == 0;
            },
            false)) {
    }
}

void DXFeed::detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detachSubscriptionAndClear(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (runIsolatedOrElse(
            [handle = static_cast<dxfg_feed_t *>(handle_.get()),
             subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscriptionAndClear(static_cast<graal_isolatethread_t *>(threadHandle),
                                                              handle, subHandle) == 0;
            },
            false)) {
    }
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(const EventTypeEnum &eventType) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::createSubscription(eventType = " + eventType.getName() + ")");
    }

    auto sub = DXFeedSubscription::create(eventType);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(std::initializer_list<EventTypeEnum> eventTypes) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::createSubscription(eventTypes = " +
                        namesToString(eventTypes.begin(), eventTypes.end()) + ")");
    }

    auto sub = DXFeedSubscription::create(eventTypes);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeed> DXFeed::create(void *feedHandle) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeed::create(" + dxfcpp::toString(feedHandle) + ")");
    }

    std::shared_ptr<DXFeed> feed{new DXFeed{}};

    auto id = ApiContext::getInstance()->getManager<DXFeedManager>()->registerEntity(feed);
    ignore_unused(id);

    feed->handle_ = JavaObjectHandle<DXFeed>(feedHandle);

    return feed;
}

void *DXFeed::getTimeSeriesPromiseImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                       std::int64_t fromTime, std::int64_t toTime) const {
    return isolated::api::IsolatedDXFeed::getTimeSeriesPromise(handle_, eventType, symbol, fromTime, toTime);
}

std::shared_ptr<EventType> DXFeed::getLastEventImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol) const {
    return isolated::api::IsolatedDXFeed::getLastEvent(handle_, symbol.toStringUnderlying(), eventType);
}

std::string DXFeed::toString() const {
    return fmt::format("DXFeed{{{}}}", handle_.toString());
}

DXFCPP_END_NAMESPACE