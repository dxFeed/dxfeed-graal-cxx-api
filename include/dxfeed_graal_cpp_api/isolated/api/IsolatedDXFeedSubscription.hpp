// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <unordered_set>

#include "../../api/DXFeedSubscription.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeedSubscription {

/*

int32_t                   dxfg_DXFeedSubscription_clear(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_attach(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
dxfg_feed_t *feed);

int32_t                   dxfg_DXFeedSubscription_detach(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
dxfg_feed_t *feed);

int32_t                   dxfg_DXFeedSubscription_isClosed(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

dxfg_event_clazz_list_t*  dxfg_DXFeedSubscription_getEventTypes(graal_isolatethread_t *thread, dxfg_subscription_t
*sub);

int32_t                   dxfg_DXFeedSubscription_containsEventType(graal_isolatethread_t *thread, dxfg_subscription_t
*sub, dxfg_event_clazz_t eventClazz);

dxfg_symbol_list*         dxfg_DXFeedSubscription_getSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_setSymbol(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
dxfg_symbol_t *symbol);

int32_t                   dxfg_DXFeedSubscription_setSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
dxfg_symbol_list *symbols);

dxfg_symbol_list*         dxfg_DXFeedSubscription_getDecoratedSymbols(graal_isolatethread_t *thread, dxfg_subscription_t
*sub);

dxfg_executor_t*          dxfg_DXFeedSubscription_getExecutor(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_setExecutor(graal_isolatethread_t *thread, dxfg_executor_t *executor);

*/

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
 * @throws std::invalid_argument if eventClassList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_list_t * */ const std::unique_ptr<EventClassList> &eventClassList);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_close` in isolation.
 *
 * @param sub The subscription's handle.
 * @throws std::invalid_argument if DXFeedSubscription's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ close(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addEventListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws std::invalid_argument if DXFeedSubscription's or DXFeedEventListener's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
addEventListener(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                 /* dxfg_feed_event_listener_t * */ const JavaObjectHandle<DXFeedEventListener> &listener);

// int32_t dxfg_DXFeedSubscription_removeEventListener(graal_isolatethread_t *thread, dxfg_subscription_t *sub,
// dxfg_feed_event_listener_t *listener);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addSymbol` in isolation.
 *
 * @param sub The subscription's handle.
 * @param symbol The subscription's symbol.
 * @throws std::invalid_argument if DXFeedSubscription's handle is invalid or the symbol is nullptr.
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
 * @throws std::invalid_argument if DXFeedSubscription's handle is invalid or the symbols is nullptr.
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
 * @throws std::invalid_argument if DXFeedSubscription's handle is invalid or the symbol is nullptr.
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
 * @throws std::invalid_argument if DXFeedSubscription's handle is invalid or the symbols is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                 /* dxfg_symbol_list * */ void *symbols);

/**
 * Calls the Graal SDK function `dxfg_DXFeedSubscription_addChangeListener` in isolation.
 *
 * @param sub The subscription's handle.
 * @param listener The listener's handle.
 * @throws std::invalid_argument if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
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
 * @throws std::invalid_argument if DXFeedSubscription's or ObservableSubscriptionChangeListener's handle
 * is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ removeChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener);

namespace DXFeedEventListener {

/**
 * Calls the Graal SDK function `dxfg_DXFeedEventListener_new` in isolation.
 * @param userFunc A user function that is used as a callback for the listener.
 * @param userData User data, which is placed each time as a callback parameter when called from listener.
 * @return The DXFeedEventListener's handle.
 *
 * @throws std::invalid_argument if userFunc is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaObjectHandle<dxfcpp::DXFeedEventListener> /* dxfg_feed_event_listener_t* */
create(/* dxfg_feed_event_listener_function */ void *userFunc, void *userData);

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()