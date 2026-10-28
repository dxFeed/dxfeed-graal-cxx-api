// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeed.hpp"
#if defined(DXFCXX_ENABLE_METRICS)
#    include "../../include/dxfeed_graal_cpp_api/internal/Metrics.hpp"
#endif

#include <dxfg_api.h>
#include <memory>
#include <string>
#include <unordered_map>
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

    // A closed subscription is not attached.
    if (!handle_ || !subscription || !subscription->handle_ || subscription->closed_) {
        return;
    }

    // After the shutdown of the library there is nothing to attach or detach.
    ignoreUnused(runIsolatedOrElse(
        [handle = static_cast<dxfg_feed_t *>(handle_.get()),
         subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
            return JavaException::throwIfMinusOne(
                dxfg_DXFeed_attachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), handle, subHandle));
        },
        0));

    {
        std::lock_guard guard{subscriptionsMutex_};

        if (closed_) {
            return;
        }

        subscriptions_.insert_or_assign(subscription.get(), subscription);
    }

    subscription->addFeed(sharedAs<const DXFeed>());

    // close() takes the feeds of the subscription after it sets the flag, so a subscription closed meanwhile either is
    // released by close() or is seen closed here.
    if (subscription->closed_) {
        releaseSubscription(subscription.get());
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

    // After the shutdown of the library there is nothing to attach or detach.
    ignoreUnused(runIsolatedOrElse(
        [handle = static_cast<dxfg_feed_t *>(handle_.get()),
         subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
            return JavaException::throwIfMinusOne(
                dxfg_DXFeed_detachSubscription(static_cast<graal_isolatethread_t *>(threadHandle), handle, subHandle));
        },
        0));

    subscription->removeFeed(this);
    releaseSubscription(subscription.get());
}

void DXFeed::detachSubscriptionAndClear(const std::shared_ptr<DXFeedSubscription> &subscription) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::detachSubscriptionAndClear(" + subscription->toString() + ")");
    }

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    // After the shutdown of the library there is nothing to attach or detach.
    ignoreUnused(runIsolatedOrElse(
        [handle = static_cast<dxfg_feed_t *>(handle_.get()),
         subHandle = static_cast<dxfg_subscription_t *>(subscription->handle_.get())](auto threadHandle) {
            return JavaException::throwIfMinusOne(dxfg_DXFeed_detachSubscriptionAndClear(
                static_cast<graal_isolatethread_t *>(threadHandle), handle, subHandle));
        },
        0));

    subscription->removeFeed(this);
    releaseSubscription(subscription.get());
}

void DXFeed::releaseSubscription(const DXFeedSubscription *subscription) const {
    std::shared_ptr<DXFeedSubscription> released{};

    {
        std::lock_guard guard{subscriptionsMutex_};

        if (const auto found = subscriptions_.find(subscription); found != subscriptions_.end()) {
            released = std::move(found->second);
            subscriptions_.erase(found);
        }
    }
}

void DXFeed::releaseSubscriptions() const {
    auto released =
        std::make_shared<std::unordered_map<const DXFeedSubscription *, std::shared_ptr<DXFeedSubscription>>>();

    {
        std::lock_guard guard{subscriptionsMutex_};

        closed_ = true;
        released->swap(subscriptions_);
    }

    for (const auto &[_, subscription] : *released) {
        subscription->removeFeed(this);
    }

    // Called from a callback (the CLOSED notification, close() of the endpoint from a listener), the subscriptions are
    // released on another thread: the destructor of a subscription closes it.
    releaseOutsideCallbacks(std::move(released));
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
        throw InvalidArgumentException("Event type " + eventType.getClassName() + " is not TimeSeries");
    }

    auto sub = RequireMakeShared<DXFeedTimeSeriesSubscription>::createShared(
        eventType, isolated::api::IsolatedDXFeed::createTimeSeriesSubscription(handle_, eventType));
    DXFeedSubscription::registerSubscription(sub);
    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedTimeSeriesSubscription>
DXFeed::createTimeSeriesSubscription(std::initializer_list<EventTypeEnum> eventTypes) const {
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