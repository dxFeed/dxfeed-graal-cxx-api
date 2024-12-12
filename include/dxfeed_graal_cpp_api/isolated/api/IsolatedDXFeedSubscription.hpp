// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXFeedSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

struct TimePeriod;

namespace isolated::api::IsolatedDXFeedSubscription {

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_new` in isolation.
 *
 * @param eventType The subscription's event type.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_t */ const EventTypeEnum &eventType);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_new2` in isolation.
 *
 * @param eventClassList The subscription's event types.
 *
 * @throws InvalidArgumentException if eventClassList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_list_t * */ const std::unique_ptr<EventClassList> &eventClassList);

// int32_t dxfg_DXFeedSubscription_attach(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_t *feed);

// int32_t dxfg_DXFeedSubscription_detach(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_t *feed);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_isClosed` in isolation.
 *
 * @param sub The subscription's handle.
 * @return `true` if subscription is closed.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ isClosed(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_close` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ close(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

// dxfg_event_clazz_list_t*  dxfg_DXFeedSubscription_getEventTypes(graal_isolatethread_t *thread, dxfg_subscription_t
// *sub);

// int32_t dxfg_DXFeedSubscription_containsEventType(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
// dxfg_event_clazz_t eventClazz);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_clear` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ clear(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_getSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<SymbolWrapper> /* dxfg_symbol_list* */
getSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_getDecoratedSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @return The subscription's decorated symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<SymbolWrapper> /* dxfg_symbol_list* */
getDecoratedSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

// int32_t dxfg_DXFeedSubscription_setSymbol(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_t
// *symbol);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_setSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ setSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_symbol_list * */ void *symbols);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addSymbol` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbol The subscription's symbol.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbol is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addSymbol(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                             /* dxfg_symbol_t * */ void *symbol);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_symbol_list * */ void *symbols);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_removeSymbol` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbol The subscription's symbol.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbol is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeSymbol(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                /* dxfg_symbol_t * */ void *symbol);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_removeSymbols` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbols The subscription's symbols.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                 /* dxfg_symbol_list * */ void *symbols);

// dxfg_executor_t* dxfg_DXFeedSubscription_getExecutor(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

// int32_t dxfg_DXFeedSubscription_setExecutor(graal_isolatethread_t *thread, dxfg_executor_t *executor);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_getAggregationPeriod` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod>
getAggregationPeriod(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_setAggregationPeriod` in isolation.
 *
 * @param sub The subscription's handle.
 * @param period The period's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's handle or period's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* int32_t */ void setAggregationPeriod(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                        /* dxfg_time_period_t * */ const JavaObjectHandle<TimePeriod> &period);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addEventListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or DXFeedEventListener's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
addEventListener(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                 /* dxfg_feed_event_listener_t * */ const JavaObjectHandle<DXFeedEventListener> &listener);

// int32_t dxfg_DXFeedSubscription_removeEventListener(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
// dxfg_feed_event_listener_t *listener);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ addChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_removeChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

/// dxfg_DXFeedSubscription_getEventsBatchLimit
std::int32_t getEventsBatchLimit(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/// dxfg_DXFeedSubscription_setEventsBatchLimit
/* int32_t */ void setEventsBatchLimit(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                       std::int32_t eventsBatchLimit);

namespace DXFeedEventListener {

/**
 * Calls the Graal SDK function `dxfg_DXFeedEventListener_new` in isolation.
 * @param userFunc A user function that is used as a callback for the listener.
 * @param userData User data, which is placed each time as a callback parameter when called from listener.
 * @return The DXFeedEventListener's handle.
 *
 * @throws InvalidArgumentException if userFunc is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<dxfcpp::DXFeedEventListener> /* dxfg_feed_event_listener_t* */
create(/* dxfg_feed_event_listener_function */ void *userFunc, void *userData);

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()