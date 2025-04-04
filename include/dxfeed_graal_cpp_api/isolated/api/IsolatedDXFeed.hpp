// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXFeed.hpp"
#include "../../api/DXFeedSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

// dxfg_feed_t* dxfg_DXFeed_getInstance(graal_isolatethread_t *thread);
// dxfg_subscription_t* dxfg_DXFeed_createSubscription(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz); dxfg_subscription_t* dxfg_DXFeed_createSubscription2(graal_isolatethread_t *thread,
// dxfg_feed_t *feed, dxfg_event_clazz_list_t *eventClazzes);

// dxfg_time_series_subscription_t* dxfg_DXFeed_createTimeSeriesSubscription(graal_isolatethread_t *thread, dxfg_feed_t
// *feed, dxfg_event_clazz_t eventClazz);
JavaObjectHandle<DXFeedSubscription> createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType);

// dxfg_time_series_subscription_t*  dxfg_DXFeed_createTimeSeriesSubscription2(graal_isolatethread_t *thread,
// dxfg_feed_t *feed, dxfg_event_clazz_list_t *eventClazzes);
JavaObjectHandle<DXFeedSubscription>
createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                             const std::unique_ptr<EventClassList> &eventClassList);

// int32_t dxfg_DXFeed_attachSubscription(graal_isolatethread_t *thread, dxfg_feed_t *feed, dxfg_subscription_t *sub);
// int32_t dxfg_DXFeed_detachSubscription(graal_isolatethread_t *thread, dxfg_feed_t *feed, dxfg_subscription_t *sub);
// int32_t dxfg_DXFeed_detachSubscriptionAndClear(graal_isolatethread_t *thread, dxfg_feed_t *feed, dxfg_subscription_t
// *sub);

// dxfg_DXFeed_getLastEventIfSubscribed
/* dxfg_event_type_t* */ std::shared_ptr<EventType>
getLastEventIfSubscribed(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                         /* dxfg_event_clazz_t */ const EventTypeEnum &eventType,
                         /* dxfg_symbol_t * */ const SymbolWrapper &symbol);

// dxfg_event_type_list* dxfg_DXFeed_getIndexedEventsIfSubscribed(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, const char *source);
std::vector<std::shared_ptr<EventType>> getIndexedEventsIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                     const EventTypeEnum &eventType,
                                                                     const SymbolWrapper &symbol,
                                                                     const IndexedEventSource &source);

// dxfg_event_type_list*  dxfg_DXFeed_getTimeSeriesIfSubscribed(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, int64_t from_time, int64_t to_time);
std::vector<std::shared_ptr<EventType>> getTimeSeriesIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType,
                                                                  const SymbolWrapper &symbol, std::int64_t fromTime,
                                                                  std::int64_t toTime);

// use dxfg_EventType_new to create an empty structure so that java tries to free up memory when replacing subjects
// dxfg_DXFeed_getLastEvent
/* int32_t */ std::shared_ptr<EventType> getLastEvent(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                                                      /* dxfg_event_type_t * */ const StringLikeWrapper &symbolName,
                                                      const EventTypeEnum &eventType);

/*
int32_t                           dxfg_DXFeed_getLastEvents(graal_isolatethread_t *thread, dxfg_feed_t *feed,
dxfg_event_type_list *events);
*/

// dxfg_promise_event_t*             dxfg_DXFeed_getLastEventPromise(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol);
void *getLastEventPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                          const SymbolWrapper &symbol);

// dxfg_promise_list*                dxfg_DXFeed_getLastEventsPromises(graal_isolatethread_t *thread, dxfg_feed_t *feed,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_list *symbols);
void *getLastEventsPromises(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType, void *symbols);

// dxfg_promise_events_t*            dxfg_DXFeed_getIndexedEventsPromise(graal_isolatethread_t *thread, dxfg_feed_t
// *feed, dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, dxfg_indexed_event_source_t* source);
void *getIndexedEventsPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                              const SymbolWrapper &symbol, const IndexedEventSource &source);

// dxfg_DXFeed_getTimeSeriesPromise
/* dxfg_promise_events_t* */ void *getTimeSeriesPromise(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                                                        /* dxfg_event_clazz_t */ const EventTypeEnum &eventType,
                                                        /* dxfg_symbol_t * */ const SymbolWrapper &symbol,
                                                        std::int64_t fromTime, std::int64_t toTime);

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()