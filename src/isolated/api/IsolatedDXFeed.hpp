// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../../include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

/// dxfg_v4_DXFeed_createTimeSeriesSubscription
JavaObjectHandle<DXFeedSubscription> createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType);

/// dxfg_v4_DXFeed_createTimeSeriesSubscriptionWithEventTypes
JavaObjectHandle<DXFeedSubscription>
createTimeSeriesSubscription(const JavaObjectHandle<DXFeed> &feed,
                             const std::unique_ptr<EventClassList> &eventClassList);

/// dxfg_v4_DXFeed_getLastEventIfSubscribed
std::shared_ptr<EventType> getLastEventIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                    const EventTypeEnum &eventType, const SymbolWrapper &symbol);

/// dxfg_v4_DXFeed_getIndexedEventsIfSubscribed
std::vector<std::shared_ptr<EventType>> getIndexedEventsIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                     const EventTypeEnum &eventType,
                                                                     const SymbolWrapper &symbol,
                                                                     const IndexedEventSource &source);

/// dxfg_v4_DXFeed_getTimeSeriesIfSubscribed
std::vector<std::shared_ptr<EventType>> getTimeSeriesIfSubscribed(const JavaObjectHandle<DXFeed> &feed,
                                                                  const EventTypeEnum &eventType,
                                                                  const SymbolWrapper &symbol, std::int64_t fromTime,
                                                                  std::int64_t toTime);

/// dxfg_v4_DXFeed_getLastEvent
std::shared_ptr<EventType> getLastEvent(const JavaObjectHandle<DXFeed> &feed, std::string_view symbolName,
                                        const EventTypeEnum &eventType);

/// dxfg_v4_DXFeed_getLastEventPromise
void *getLastEventPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                          const SymbolWrapper &symbol);

/// dxfg_v4_DXFeed_getLastEventsPromises
void *getLastEventsPromises(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType, void *symbols);

/// dxfg_v4_DXFeed_getIndexedEventsPromise
void *getIndexedEventsPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                              const SymbolWrapper &symbol, const IndexedEventSource &source);

/// dxfg_v4_DXFeed_getTimeSeriesPromise
void *getTimeSeriesPromise(const JavaObjectHandle<DXFeed> &feed, const EventTypeEnum &eventType,
                           const SymbolWrapper &symbol, std::int64_t fromTime, std::int64_t toTime);

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()