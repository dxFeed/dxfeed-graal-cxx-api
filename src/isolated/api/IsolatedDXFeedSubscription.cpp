// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeedSubscription.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeedSubscription {

JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_t */ const EventTypeEnum &eventType) {
    return JavaObjectHandle<dxfcpp::DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedSubscription_new, dxfcpp::bit_cast<dxfg_event_clazz_t>(eventType.getId())));
}

JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_list_t * */ const std::unique_ptr<EventClassList> &eventClassList) {
    if (!eventClassList) {
        throw std::invalid_argument("The eventClassList is nullptr");
    }

    return JavaObjectHandle<DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedSubscription_new2, static_cast<dxfg_event_clazz_list_t *>(eventClassList->getHandle())));
}

void /* int32_t */ close(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXFeedSubscription_close`. The `sub` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_close,
                                           static_cast<dxfg_subscription_t *>(sub.get()));
}

void /* int32_t */
addEventListener(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                 /* dxfg_feed_event_listener_t * */ const JavaObjectHandle<dxfcpp::DXFeedEventListener> &listener) {
    if (!sub) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXFeedSubscription_addEventListener`. The `sub` handle is invalid");
    }

    if (!listener) {
        throw std::invalid_argument("Unable to execute function `dxfg_DXFeedSubscription_addEventListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addEventListener,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_feed_event_listener_t *>(listener.get()));
}

void /* int32_t */ addSymbol(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                             /* dxfg_symbol_t * */ void *symbol) {
    if (!sub) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXFeedSubscription_addSymbol`. The `sub` handle is invalid");
    }

    if (!symbol) {
        throw std::invalid_argument("Unable to execute function `dxfg_DXFeedSubscription_addSymbol`. The "
                                    "`symbol` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addSymbol,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_t *>(symbol));
}

void /* int32_t */ addChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXFeedSubscription_addChangeListener`. The `sub` handle is invalid");
    }

    if (!listener) {
        throw std::invalid_argument("Unable to execute function `dxfg_DXFeedSubscription_addChangeListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_DXFeedSubscription_addChangeListener, static_cast<dxfg_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}

void /* int32_t */ removeChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw std::invalid_argument("Unable to execute function `dxfg_DXFeedSubscription_removeChangeListener`. "
                                    "The `sub` handle is invalid");
    }

    if (!listener) {
        throw std::invalid_argument("Unable to execute function `dxfg_DXFeedSubscription_removeChangeListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_DXFeedSubscription_removeChangeListener, static_cast<dxfg_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}

namespace DXFeedEventListener {

JavaObjectHandle<dxfcpp::DXFeedEventListener> /* dxfg_feed_event_listener_t* */
create(/* dxfg_feed_event_listener_function */ void *userFunc, void *userData) {
    if (!userFunc) {
        throw std::invalid_argument("Unable to create DXFeedEventListener. The `userFunc` parameter is nullptr");
    }

    return JavaObjectHandle<dxfcpp::DXFeedEventListener>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedEventListener_new, dxfcpp::bit_cast<dxfg_feed_event_listener_function>(userFunc), userData));
}

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

DXFCPP_END_NAMESPACE
