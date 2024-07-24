// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../internal/Conf.hpp"

#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../api/osub/ObservableSubscriptionChangeListener.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedObservableSubscriptionChangeListener {
/**
 * Calls the Graal SDK function `dxfg_ObservableSubscriptionChangeListener_new` in isolation.
 * @param functionSymbolsAdded A user function that is used as a `symbolsAdded` callback for the listener.
 * @param functionSymbolsRemoved A user function that is used as a `symbolsRemoved` callback for the listener.
 * @param functionSubscriptionClosed A user function that is used as a `subscriptionClosed` callback for the listener.
 * @param userData User data, which is placed each time as a callback parameter when called from listener.
 * @return The ObservableSubscriptionChangeListener's handle.
 *
 * @throws InvalidArgumentException if functionSymbolsAdded or functionSymbolsRemoved or functionSubscriptionClosed is
 * nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_observable_subscription_change_listener_t* */ JavaObjectHandle<dxfcpp::ObservableSubscriptionChangeListener>
create(/* dxfg_ObservableSubscriptionChangeListener_function_symbolsAdded */ void *functionSymbolsAdded,
       /* dxfg_ObservableSubscriptionChangeListener_function_symbolsRemoved */ void *functionSymbolsRemoved,
       /* dxfg_ObservableSubscriptionChangeListener_function_subscriptionClosed */ void *functionSubscriptionClosed,
       void *userData);
} // namespace isolated::api::IsolatedObservableSubscriptionChangeListener

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()