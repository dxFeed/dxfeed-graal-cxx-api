// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeed.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

// dxfg_time_series_subscription_t* dxfg_DXFeed_createTimeSeriesSubscription(graal_isolatethread_t *thread, dxfg_feed_t
// *feed, dxfg_event_clazz_t eventClazz);
JavaObjectHandle<DXFeedSubscription> createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_createTimeSeriesSubscription`. The `feed` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_createTimeSeriesSubscription, static_cast<dxfg_feed_t *>(feed.get()),
        dxfcpp::bit_cast<dxfg_event_clazz_t>(eventType.getId())));
}

// dxfg_time_series_subscription_t*  dxfg_DXFeed_createTimeSeriesSubscription2(graal_isolatethread_t *thread,
// dxfg_feed_t *feed, dxfg_event_clazz_list_t *eventClazzes);
JavaObjectHandle<DXFeedSubscription>
createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                             const std::unique_ptr<EventClassList> &eventClassList) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_createTimeSeriesSubscription2`. The `feed` handle is invalid");
    }

    if (!eventClassList) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_createTimeSeriesSubscription2`. The `eventClassList` is nullptr");
    }

    return JavaObjectHandle<DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_createTimeSeriesSubscription2, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_list_t *>(eventClassList->getHandle())));
}

// dxfg_DXFeed_getLastEventIfSubscribed
/* dxfg_event_type_t* */ std::shared_ptr<EventType>
getLastEventIfSubscribed(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                         /* dxfg_event_clazz_t */ const EventTypeEnum &eventType,
                         /* dxfg_symbol_t * */ const SymbolWrapper &symbol) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getLastEventIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    const auto e = static_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_getLastEventIfSubscribed, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(graalSymbol.get())));

    if (!e) {
        return {};
    }

    auto u = event::IsolatedEventType::toUnique(e);

    return EventMapper::fromGraal(u.get());
}

// dxfg_event_type_list* dxfg_DXFeed_getIndexedEventsIfSubscribed(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, const char *source);
std::vector<std::shared_ptr<EventType>> getIndexedEventsIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                     const EventTypeEnum &eventType,
                                                                     const SymbolWrapper &symbol,
                                                                     const IndexedEventSource &source) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getIndexedEventsIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    const auto list = static_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_getIndexedEventsIfSubscribed, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(graalSymbol.get()),
        source.toString().c_str()));

    if (!list) {
        return {};
    }

    auto u = event::IsolatedEventTypeList::toUnique(list);

    return EventMapper::fromGraalList(u.get());
}

// dxfg_event_type_list*  dxfg_DXFeed_getTimeSeriesIfSubscribed(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, int64_t from_time, int64_t to_time);
std::vector<std::shared_ptr<EventType>> getTimeSeriesIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType,
                                                                  const SymbolWrapper &symbol, std::int64_t fromTime,
                                                                  std::int64_t toTime) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getTimeSeriesIfSubscribed`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    const auto list = static_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_DXFeed_getTimeSeriesIfSubscribed, static_cast<dxfg_feed_t *>(feed.get()),
                                          static_cast<dxfg_event_clazz_t>(eventType.getId()),
                                          static_cast<dxfg_symbol_t *>(graalSymbol.get()), fromTime, toTime));

    auto u = event::IsolatedEventTypeList::toUnique(list);

    return EventMapper::fromGraalList(u.get());
}

// dxfg_DXFeed_getLastEvent
/* int32_t */ std::shared_ptr<EventType> getLastEvent(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                                                      /* dxfg_event_type_t * */ const StringLikeWrapper &symbolName,
                                                      const EventTypeEnum &eventType) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getLastEvent`. The `feed` handle is invalid");
    }

    const auto e = event::IsolatedEventType::toUnique(event::IsolatedEventType::create(symbolName, eventType));

    runGraalFunctionAndThrowIfMinusOne(dxfg_DXFeed_getLastEvent, static_cast<dxfg_feed_t *>(feed.get()),
                                       static_cast<dxfg_event_type_t *>(e.get()));

    return EventMapper::fromGraal(e.get());
}

/*
int32_t                           dxfg_DXFeed_getLastEvents(graal_isolatethread_t *thread, dxfg_feed_t *feed,
dxfg_event_type_list *events); dxfg_promise_event_t*             dxfg_DXFeed_getLastEventPromise(graal_isolatethread_t
*thread, dxfg_feed_t *feed, dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol); dxfg_promise_list*
dxfg_DXFeed_getLastEventsPromises(graal_isolatethread_t *thread, dxfg_feed_t *feed, dxfg_event_clazz_t eventClazz,
dxfg_symbol_list *symbols); dxfg_promise_events_t*            dxfg_DXFeed_getIndexedEventsPromise(graal_isolatethread_t
*thread, dxfg_feed_t *feed, dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, dxfg_indexed_event_source_t* source);

*/

// dxfg_promise_event_t*             dxfg_DXFeed_getLastEventPromise(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol);
void *getLastEventPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                          const SymbolWrapper &symbol) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getLastEventPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_getLastEventPromise, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(graalSymbol.get())));
}

// dxfg_promise_list*                dxfg_DXFeed_getLastEventsPromises(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_list *symbols);
void *getLastEventsPromises(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType, void *symbols) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getLastEventsPromises`. The `feed` handle is invalid");
    }

    if (!symbols) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getLastEventsPromises`. The `symbols` is nullptr");
    }

    return runGraalFunctionAndThrowIfNullptr(dxfg_DXFeed_getLastEventsPromises, static_cast<dxfg_feed_t *>(feed.get()),
                                             static_cast<dxfg_event_clazz_t>(eventType.getId()),
                                             static_cast<dxfg_symbol_list *>(symbols));
}

// dxfg_promise_events_t*            dxfg_DXFeed_getIndexedEventsPromise(graal_isolatethread_t *thread, dxfg_feed_t
// *feed, dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, dxfg_indexed_event_source_t* source);
void *getIndexedEventsPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                              const SymbolWrapper &symbol, const IndexedEventSource &source) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getIndexedEventsPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();
    auto graalSource = source.toGraalUnique();

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_getIndexedEventsPromise, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(graalSymbol.get()),
        static_cast<dxfg_indexed_event_source_t *>(graalSource.get())));
}

/* dxfg_promise_events_t* */ void *getTimeSeriesPromise(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                                                        /* dxfg_event_clazz_t */ const EventTypeEnum &eventType,
                                                        /* dxfg_symbol_t * */ const SymbolWrapper &symbol,
                                                        std::int64_t fromTime, std::int64_t toTime) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getTimeSeriesPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return dxfcpp::bit_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_DXFeed_getTimeSeriesPromise, static_cast<dxfg_feed_t *>(feed.get()),
                                          static_cast<dxfg_event_clazz_t>(eventType.getId()),
                                          static_cast<dxfg_symbol_t *>(graalSymbol.get()), fromTime, toTime));
}

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE