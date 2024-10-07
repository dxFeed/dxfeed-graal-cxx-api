// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedTxModelListener.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTxModelListener {

// typedef void (*dxfg_TxModelListener_function_eventsReceived)(graal_isolatethread_t* thread, dxfg_indexed_event_source_t* source, dxfg_event_type_list* events, int32_t /* boolean */ isSnapshot, void* user_data);


// JavaOjectHandle<TxModelListener> dxfg_TxModelListener_new
/* dxfg_tx_model_listener_t* */ void* create(/* dxfg_TxModelListener_function_eventsReceived */ void* functionEventsReceived, void* userData) {

}

}

DXFCPP_END_NAMESPACE