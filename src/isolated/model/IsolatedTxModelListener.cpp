// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/model/IsolatedTxModelListener.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTxModelListener {

// typedef void (*dxfg_TxModelListener_function_eventsReceived)(graal_isolatethread_t* thread,
// dxfg_indexed_event_source_t* source, dxfg_event_type_list* events, int32_t /* boolean */ isSnapshot, void*
// user_data);

// dxfg_tx_model_listener_t* dxfg_TxModelListener_new(graal_isolatethread_t* thread,
// dxfg_TxModelListener_function_eventsReceived  function_eventsReceived, void* user_data);
JavaObjectHandle<TxModelListenerTag> create(void *functionEventsReceived, void *userData) {
    if (!functionEventsReceived) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TxModelListener_new`. The `functionEventsReceived` is nullptr");
    }

    return JavaObjectHandle<TxModelListenerTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TxModelListener_new,
        dxfcpp::bit_cast<dxfg_TxModelListener_function_eventsReceived>(functionEventsReceived), userData)};
}

} // namespace isolated::model::IsolatedTxModelListener

DXFCPP_END_NAMESPACE