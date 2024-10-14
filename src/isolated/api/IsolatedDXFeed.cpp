// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeed.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

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

    auto result = dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeed_getLastEventPromise, static_cast<dxfg_feed_t *>(feed.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(graalSymbol.get())));

    return result;
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

    auto result = dxfcpp::bit_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_DXFeed_getTimeSeriesPromise, static_cast<dxfg_feed_t *>(feed.get()),
                                          static_cast<dxfg_event_clazz_t>(eventType.getId()),
                                          static_cast<dxfg_symbol_t *>(graalSymbol.get()), fromTime, toTime));

    return result;
}

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE