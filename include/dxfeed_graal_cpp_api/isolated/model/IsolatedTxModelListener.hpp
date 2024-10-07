// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

//#include "../../model/TxModelListener.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTxModelListener {

// typedef void (*dxfg_TxModelListener_function_eventsReceived)(graal_isolatethread_t* thread, dxfg_indexed_event_source_t* source, dxfg_event_type_list* events, int32_t /* boolean */ isSnapshot, void* user_data);


// JavaOjectHandle<TxModelListener> dxfg_TxModelListener_new
/* dxfg_tx_model_listener_t* */ void* create(/* dxfg_TxModelListener_function_eventsReceived */ void* functionEventsReceived, void* userData);

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()