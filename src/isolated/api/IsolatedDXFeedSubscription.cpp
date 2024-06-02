// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeedSubscription.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeedSubscription {

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

} // namespace isolated::api::IsolatedDXFeedSubscription

DXFCPP_END_NAMESPACE
