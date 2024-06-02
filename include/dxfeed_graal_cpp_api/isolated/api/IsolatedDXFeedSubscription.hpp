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
dxfg_subscription_t*      dxfg_DXFeedSubscription_new(graal_isolatethread_t *thread, dxfg_event_clazz_t eventClazz);

dxfg_subscription_t*      dxfg_DXFeedSubscription_new2(graal_isolatethread_t *thread, dxfg_event_clazz_list_t *eventClazzes);

int32_t                   dxfg_DXFeedSubscription_close(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_addEventListener(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_event_listener_t *listener);

int32_t                   dxfg_DXFeedSubscription_removeEventListener(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_event_listener_t *listener);

int32_t                   dxfg_DXFeedSubscription_addSymbol(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_t *symbol);

int32_t                   dxfg_DXFeedSubscription_addSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_list *symbols);

int32_t                   dxfg_DXFeedSubscription_removeSymbol(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_t *symbol);

int32_t                   dxfg_DXFeedSubscription_removeSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_list *symbols);

int32_t                   dxfg_DXFeedSubscription_clear(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_attach(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_t *feed);

int32_t                   dxfg_DXFeedSubscription_detach(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_feed_t *feed);

int32_t                   dxfg_DXFeedSubscription_isClosed(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

dxfg_event_clazz_list_t*  dxfg_DXFeedSubscription_getEventTypes(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_containsEventType(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_event_clazz_t eventClazz);

dxfg_symbol_list*         dxfg_DXFeedSubscription_getSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_setSymbol(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_t *symbol);

int32_t                   dxfg_DXFeedSubscription_setSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub, dxfg_symbol_list *symbols);

dxfg_symbol_list*         dxfg_DXFeedSubscription_getDecoratedSymbols(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

dxfg_executor_t*          dxfg_DXFeedSubscription_getExecutor(graal_isolatethread_t *thread, dxfg_subscription_t *sub);

int32_t                   dxfg_DXFeedSubscription_setExecutor(graal_isolatethread_t *thread, dxfg_executor_t *executor);

*/


// dxfg_DXFeedSubscription_new
JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */ create(/* dxfg_event_clazz_t */ const EventTypeEnum &eventType);

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

// dxfg_feed_event_listener_t* dxfg_DXFeedEventListener_new(graal_isolatethread_t *thread, dxfg_feed_event_listener_function user_func, void *user_data);

}

}

DXFCPP_END_NAMESPACE


DXFCXX_DISABLE_MSC_WARNINGS_POP()