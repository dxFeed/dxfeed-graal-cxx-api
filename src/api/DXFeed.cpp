// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeed.hpp"

#include <dxfg_api.h>
#include <memory>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<DXFeed> DXFeed::getInstance() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeed::getInstance()");
    }

    return DXEndpoint::getInstance()->getFeed();
}

void DXFeed::attachSubscription(const std::shared_ptr<DXFeedSubscription> &subscription) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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

void DXFeed::detachSubscription(const std::shared_ptr<DXFeedSubscription> &subscription) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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

void DXFeed::detachSubscriptionAndClear(const std::shared_ptr<DXFeedSubscription> &subscription) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(const EventTypeEnum &eventType) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::createSubscription(eventType = " + eventType.getName() + ")");
    }

    auto sub = DXFeedSubscription::create(eventType);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(std::initializer_list<EventTypeEnum> eventTypes) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::createSubscription(eventTypes = " +
                        namesToString(eventTypes.begin(), eventTypes.end()) + ")");
    }

    auto sub = DXFeedSubscription::create(eventTypes);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedTimeSeriesSubscription>
DXFeed::createTimeSeriesSubscription(const EventTypeEnum &eventType) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::createTimeSeriesSubscription(eventType = " + eventType.getName() + ")");
    }

    if (!eventType.isTimeSeries()) {
        throw InvalidArgumentException("Event type " +
                                       eventType.getClassName() + " is not TimeSeries");
    }

    auto sub = RequireMakeShared<DXFeedTimeSeriesSubscription>::createShared(
        eventType, isolated::api::IsolatedDXFeed::createTimeSeriesSubscription(handle_, eventType));
    const auto id = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>()->registerEntity(sub);

    ignoreUnused(id);
    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedTimeSeriesSubscription>
DXFeed::createTimeSeriesSubscription(std::initializer_list<EventTypeEnum> eventTypes) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::createTimeSeriesSubscription(eventTypes = " +
                        namesToString(eventTypes.begin(), eventTypes.end()) + ")");
    }

    return createTimeSeriesSubscription(eventTypes.begin(), eventTypes.end());
}

std::shared_ptr<DXFeed> DXFeed::create(void *feedHandle) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeed::create(" + dxfcpp::toString(feedHandle) + ")");
    }

    std::shared_ptr<DXFeed> feed{new DXFeed{}};

    const auto id = ApiContext::getInstance()->getManager<EntityManager<DXFeed>>()->registerEntity(feed);
    ignoreUnused(id);

    feed->handle_ = JavaObjectHandle<DXFeed>(feedHandle);

    return feed;
}

void *DXFeed::getLastEventPromiseImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol) const {
    return isolated::api::IsolatedDXFeed::getLastEventPromise(handle_, eventType, symbol);
}

void *DXFeed::getLastEventsPromisesImpl(const EventTypeEnum &eventType, void *graalSymbolList) const {
    return isolated::api::IsolatedDXFeed::getLastEventsPromises(handle_, eventType, graalSymbolList);
}

void *DXFeed::getIndexedEventsPromiseImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                          const IndexedEventSource &source) const {
    return isolated::api::IsolatedDXFeed::getIndexedEventsPromise(handle_, eventType, symbol, source);
}

void *DXFeed::getTimeSeriesPromiseImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                       std::int64_t fromTime, std::int64_t toTime) const {
    return isolated::api::IsolatedDXFeed::getTimeSeriesPromise(handle_, eventType, symbol, fromTime, toTime);
}

std::shared_ptr<EventType> DXFeed::getLastEventIfSubscribedImpl(const EventTypeEnum &eventType,
                                                                const SymbolWrapper &symbol) const {
    return isolated::api::IsolatedDXFeed::getLastEventIfSubscribed(handle_, eventType, symbol);
}

std::vector<std::shared_ptr<EventType>>
DXFeed::getIndexedEventsIfSubscribedImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                         const IndexedEventSource &source) const {
    return isolated::api::IsolatedDXFeed::getIndexedEventsIfSubscribed(handle_, eventType, symbol, source);
}

std::vector<std::shared_ptr<EventType>> DXFeed::getTimeSeriesIfSubscribedImpl(const EventTypeEnum &eventType,
                                                                              const SymbolWrapper &symbol,
                                                                              std::int64_t fromTime,
                                                                              std::int64_t toTime) const {
    return isolated::api::IsolatedDXFeed::getTimeSeriesIfSubscribed(handle_, eventType, symbol, fromTime, toTime);
}

JavaObjectHandle<DXFeedSubscription>
DXFeed::createTimeSeriesSubscriptionHandleFromEventClassList(const std::unique_ptr<EventClassList> &list) const {
    return isolated::api::IsolatedDXFeed::createTimeSeriesSubscription(handle_, list);
}

DXFeed::DXFeed() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeed()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeed", 1);
#endif
}

DXFeed::~DXFeed() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeed{" + handle_.toString() + "}::~DXFeed()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeed", -1);
#endif
}

std::string DXFeed::toString() const {
    return std::string("DXFeed{") + handle_.toString() + "}";
}

DXFCPP_END_NAMESPACE