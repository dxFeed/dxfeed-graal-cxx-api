// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp"

#include <cstdint>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct TimePeriod;

namespace isolated::api::IsolatedDXFeedSubscription {

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_new` in isolation.
 *
 * @param eventType The subscription's event type.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<DXFeedSubscription> /* dxfg_v4_subscription_t* */
create(/* dxfg_v4_event_clazz_t */ const EventTypeEnum &eventType);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_newWithEventTypes` in isolation.
 *
 * @param eventClassList The subscription's event types.
 *
 * @throws InvalidArgumentException if eventClassList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<DXFeedSubscription> /* dxfg_v4_subscription_t* */
create(/* dxfg_v4_event_clazz_list * */ const std::unique_ptr<EventClassList> &eventClassList);

// dxfg_v4_DXFeedSubscription_attach

// dxfg_v4_DXFeedSubscription_detach

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_isClosed` in isolation.
 *
 * @param sub The subscription's handle.
 * @return `true` if subscription is closed.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ isClosed(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_close` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ close(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

// dxfg_v4_DXFeedSubscription_getEventTypes

// dxfg_v4_DXFeedSubscription_containsEventType

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_clear` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ clear(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_getSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<SymbolWrapper> /* dxfg_v4_symbol_list* */
getSymbols(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_getDecoratedSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The subscription's decorated symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<SymbolWrapper> /* dxfg_v4_symbol_list* */
getDecoratedSymbols(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

// dxfg_v4_DXFeedSubscription_setSymbol

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_setSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ setSymbols(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_v4_symbol_list * */ void *symbols);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_addSymbol` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbol The subscription's symbol.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbol is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addSymbol(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                             /* dxfg_v4_symbol_t * */ void *symbol);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_addSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addSymbols(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_v4_symbol_list * */ void *symbols);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_removeSymbol` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbol The subscription's symbol.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbol is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeSymbol(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                /* dxfg_v4_symbol_t * */ void *symbol);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_removeSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeSymbols(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                 /* dxfg_v4_symbol_list * */ void *symbols);

// dxfg_v4_DXFeedSubscription_getExecutor

// dxfg_v4_DXFeedSubscription_setExecutor

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_getAggregationPeriod` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_v4_time_period_t* */ JavaObjectHandle<TimePeriod>
getAggregationPeriod(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_setAggregationPeriod` in isolation.
 *
 * @param sub The subscription's handle.
 * @param period The period's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle or period's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* int32_t */ void setAggregationPeriod(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                        /* dxfg_v4_time_period_t * */ const JavaObjectHandle<TimePeriod> &period);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_getAggregationPeriodInfo` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The minimum and the maximum aggregation period in milliseconds, negative if unknown.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::pair<std::int64_t, std::int64_t>
getAggregationPeriodInfo(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_getTotalDroppedEvents` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The number of the events dropped because of a buffer overflow since the subscription was created.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::int64_t getTotalDroppedEvents(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_addEventListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or DXFeedEventListener's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
addEventListener(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                 /* dxfg_v4_feed_event_listener_t * */ const JavaObjectHandle<DXFeedEventListener> &listener);

// dxfg_v4_DXFeedSubscription_removeEventListener

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_addChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addChangeListener(
    /* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_v4_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedSubscription_removeChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeChangeListener(
    /* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_v4_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

/// dxfg_v4_DXFeedSubscription_getEventsBatchLimit
std::int32_t getEventsBatchLimit(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/// dxfg_v4_DXFeedSubscription_setEventsBatchLimit
/* int32_t */ void setEventsBatchLimit(/* dxfg_v4_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                       std::int32_t eventsBatchLimit);

namespace DXFeedEventListener {

/**
 * Calls the Graal SDK function `dxfg_v4_DXFeedEventListener_new` in isolation.
 * @param userFunc A user function that is used as a callback for the listener.
 * @param userData User data, which is placed each time as a callback parameter when called from listener.
 * @return The DXFeedEventListener's handle.
 *
 * @throws InvalidArgumentException if userFunc is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<dxfcpp::DXFeedEventListener> /* dxfg_v4_feed_event_listener_t* */
create(/* dxfg_v4_feed_event_listener_function */ void *userFunc, void *userData);

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

namespace isolated::api::IsolatedDXFeedTimeSeriesSubscription {
// dxfg_v4_DXFeedTimeSeriesSubscription_setFromTime
void setFromTime(const JavaObjectHandle<DXFeedSubscription> &sub, std::int64_t fromTime);

} // namespace isolated::api::IsolatedDXFeedTimeSeriesSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()