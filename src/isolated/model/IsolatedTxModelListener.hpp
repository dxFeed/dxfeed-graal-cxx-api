// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/model/TxModelListener.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTxModelListener {

// typedef void (*dxfg_v4_TxModelListener_function_eventsReceived)(graal_isolatethread_t *thread,
// dxfg_v4_indexed_event_source_t *source, dxfg_v4_event_type_list *events, int32_t isSnapshot, void *user_data);

// int32_t dxfg_v4_TxModelListener_new(graal_isolatethread_t *thread,
// dxfg_v4_TxModelListener_function_eventsReceived function_eventsReceived, void *user_data,
// DXFG_OUT dxfg_v4_tx_model_listener_t **txModelListener);
JavaObjectHandle<TxModelListenerTag>
create(void * /* dxfg_v4_TxModelListener_function_eventsReceived */ functionEventsReceived, void *userData);

} // namespace isolated::model::IsolatedTxModelListener

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()