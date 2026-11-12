// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDXFeed.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

/// dxfg_v4_DXFeed_createTimeSeriesSubscription
JavaObjectHandle<DXFeedSubscription> createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_createTimeSeriesSubscription`. The `feed` handle is invalid");
    }

    return JavaObjectHandle<DXFeedSubscription>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_createTimeSeriesSubscription, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId())));
}

/// dxfg_v4_DXFeed_createTimeSeriesSubscriptionWithEventTypes
JavaObjectHandle<DXFeedSubscription>
createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                             const std::unique_ptr<EventClassList> &eventClassList) {
    if (!feed) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_DXFeed_createTimeSeriesSubscriptionWithEventTypes`. The `feed` handle "
                                       "is invalid");
    }

    if (!eventClassList) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_DXFeed_createTimeSeriesSubscriptionWithEventTypes`. The "
                                       "`eventClassList` is nullptr");
    }

    return JavaObjectHandle<DXFeedSubscription>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_createTimeSeriesSubscriptionWithEventTypes, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_list *>(eventClassList->getHandle())));
}

/// dxfg_v4_DXFeed_getLastEventIfSubscribed
std::shared_ptr<EventType> getLastEventIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                    const EventTypeEnum &eventType, const SymbolWrapper &symbol) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getLastEventIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    // NULL is a valid result: there is no subscription.
    const auto e = static_cast<void *>(runGraalV4FunctionWithResult(
        dxfg_v4_DXFeed_getLastEventIfSubscribed, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get())));

    if (!e) {
        return {};
    }

    auto u = event::IsolatedEventType::toUnique(e);

    return EventMapper::fromGraal(u.get());
}

/// dxfg_v4_DXFeed_getIndexedEventsIfSubscribed
std::vector<std::shared_ptr<EventType>> getIndexedEventsIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                     const EventTypeEnum &eventType,
                                                                     const SymbolWrapper &symbol,
                                                                     const IndexedEventSource &source) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getIndexedEventsIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    // An empty list is a valid result and is released like a non-empty one.
    const auto list = static_cast<void *>(runGraalV4FunctionWithResult(
        dxfg_v4_DXFeed_getIndexedEventsIfSubscribed, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()),
        SdkString{source.toString(), "DXFeed::getIndexedEventsIfSubscribed(source)"}.c_str()));

    if (!list) {
        return {};
    }

    auto u = event::IsolatedEventTypeList::toUnique(list);

    return EventMapper::fromGraalList(u.get());
}

/// dxfg_v4_DXFeed_getTimeSeriesIfSubscribed
std::vector<std::shared_ptr<EventType>> getTimeSeriesIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType,
                                                                  const SymbolWrapper &symbol, std::int64_t fromTime,
                                                                  std::int64_t toTime) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getTimeSeriesIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    // An empty list is a valid result and is released like a non-empty one.
    const auto list = static_cast<void *>(runGraalV4FunctionWithResult(
        dxfg_v4_DXFeed_getTimeSeriesIfSubscribed, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()),
        fromTime, toTime));

    if (!list) {
        return {};
    }

    auto u = event::IsolatedEventTypeList::toUnique(list);

    return EventMapper::fromGraalList(u.get());
}

/// dxfg_v4_DXFeed_getLastEvent
std::shared_ptr<EventType> getLastEvent(const JavaObjectHandle<DXFeed> &feed, std::string_view symbolName,
                                        const EventTypeEnum &eventType) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getLastEvent`. The `feed` handle is invalid");
    }

    const auto e = event::IsolatedEventType::toUnique(event::IsolatedEventType::create(symbolName, eventType));

    // Only reads the given event and writes a new one: the last event, or a copy of the given one when it is missing.
    const auto u = event::IsolatedEventType::toUnique(
        static_cast<void *>(runGraalV4FunctionWithNonNullResult(dxfg_v4_DXFeed_getLastEvent,
                                                                static_cast<dxfg_v4_feed_t *>(feed.get()),
                                                                static_cast<const dxfg_v4_event_type_t *>(e.get()))));

    return EventMapper::fromGraal(u.get());
}

/// dxfg_v4_DXFeed_getLastEventPromise
void *getLastEventPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                          const SymbolWrapper &symbol) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getLastEventPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return static_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_getLastEventPromise, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get())));
}

/// dxfg_v4_DXFeed_getLastEventsPromises
void *getLastEventsPromises(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType, void *symbols) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getLastEventsPromises`. The `feed` handle is invalid");
    }

    if (!symbols) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getLastEventsPromises`. The `symbols` is nullptr");
    }

    return static_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_getLastEventsPromises, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_list *>(symbols)));
}

/// dxfg_v4_DXFeed_getIndexedEventsPromise
void *getIndexedEventsPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                              const SymbolWrapper &symbol, const IndexedEventSource &source) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getIndexedEventsPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();
    auto graalSource = source.toGraalUnique();

    return static_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_getIndexedEventsPromise, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()),
        static_cast<dxfg_v4_indexed_event_source_t *>(graalSource.get())));
}

/// dxfg_v4_DXFeed_getTimeSeriesPromise
void *getTimeSeriesPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                           const SymbolWrapper &symbol, std::int64_t fromTime, std::int64_t toTime) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXFeed_getTimeSeriesPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return static_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeed_getTimeSeriesPromise, static_cast<dxfg_v4_feed_t *>(feed.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()), static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()),
        fromTime, toTime));
}

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE