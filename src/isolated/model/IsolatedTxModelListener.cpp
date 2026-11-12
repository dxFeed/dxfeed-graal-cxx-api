// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedTxModelListener.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTxModelListener {

JavaObjectHandle<TxModelListenerTag> create(void *functionEventsReceived, void *userData) {
    if (!functionEventsReceived) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_TxModelListener_new`. The `functionEventsReceived` is nullptr");
    }

    // The listener functions are experimental, as the models are.
    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<TxModelListenerTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TxModelListener_new,
        dxfcpp::bit_cast<dxfg_v4_TxModelListener_function_eventsReceived>(functionEventsReceived), userData)};
}

} // namespace isolated::model::IsolatedTxModelListener

DXFCPP_END_NAMESPACE