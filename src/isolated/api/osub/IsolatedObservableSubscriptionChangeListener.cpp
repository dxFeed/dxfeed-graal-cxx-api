// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../../include/dxfeed_graal_cpp_api/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"
#include "../../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedObservableSubscriptionChangeListener {

JavaObjectHandle<ObservableSubscriptionChangeListener>
create(/* dxfg_ObservableSubscriptionChangeListener_function_symbolsAdded */ void *functionSymbolsAdded,
       /* dxfg_ObservableSubscriptionChangeListener_function_symbolsRemoved */ void *functionSymbolsRemoved,
       /* dxfg_ObservableSubscriptionChangeListener_function_subscriptionClosed */ void *functionSubscriptionClosed,
       void *userData) {
    if (!functionSymbolsAdded) {
        throw InvalidArgumentException(
            "Unable to create ObservableSubscriptionChangeListener. The `functionSymbolsAdded` parameter is nullptr");
    }

    if (!functionSymbolsRemoved) {
        throw InvalidArgumentException(
            "Unable to create ObservableSubscriptionChangeListener. The `functionSymbolsRemoved` parameter is nullptr");
    }

    if (!functionSubscriptionClosed) {
        throw InvalidArgumentException("Unable to create ObservableSubscriptionChangeListener. The "
                                       "`functionSubscriptionClosed` parameter is nullptr");
    }

    return JavaObjectHandle<ObservableSubscriptionChangeListener>(runGraalFunctionAndThrowIfNullptr(
        dxfg_ObservableSubscriptionChangeListener_new,
        dxfcpp::bit_cast<dxfg_ObservableSubscriptionChangeListener_function_symbolsAdded>(functionSymbolsAdded),
        dxfcpp::bit_cast<dxfg_ObservableSubscriptionChangeListener_function_symbolsRemoved>(functionSymbolsRemoved),
        dxfcpp::bit_cast<dxfg_ObservableSubscriptionChangeListener_function_subscriptionClosed>(
            functionSubscriptionClosed),
        userData));
}

} // namespace isolated::api::IsolatedObservableSubscriptionChangeListener

DXFCPP_END_NAMESPACE