// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

//#include "../../model/IndexedTxModel.hpp"

#include <cstdint>
#include <unordered_set>
#include <type_traits>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

namespace isolated::model::IsolatedIndexedTxModel {

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_newBuilder(graal_isolatethread_t* thread, dxfg_event_clazz_t eventType);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ newBuilder(const EventTypeEnum &eventType);

//dxfg_indexed_event_source_list* dxfg_IndexedTxModel_getSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
std::unordered_set<std::reference_wrapper<const IndexedEventSource>> getSources(void* /* const JavaObjectHandle<IndexedTxModel>& */ source);

//int32_t dxfg_IndexedTxModel_setSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_indexed_event_source_list* sources);
void setSources(void* /* const JavaObjectHandle<IndexedTxModel>& */ source, void* sources);

//int32_t dxfg_IndexedTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isBatchProcessing(void* /* const JavaObjectHandle<IndexedTxModel>& */ source);

//int32_t dxfg_IndexedTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isSnapshotProcessing(void* /* const JavaObjectHandle<IndexedTxModel>& */ source);

//int32_t dxfg_IndexedTxModel_attach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t* feed);
void attach(void* /* const JavaObjectHandle<IndexedTxModel>& */ source, const JavaObjectHandle<DXFeed>& feed);

//int32_t dxfg_IndexedTxModel_detach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t* feed);
void detach(void* /* const JavaObjectHandle<IndexedTxModel>& */ source, const JavaObjectHandle<DXFeed>& feed);

//int32_t dxfg_IndexedTxModel_close(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
void close(void* /* const JavaObjectHandle<IndexedTxModel>& */ source);

namespace Builder {

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, dxfg_indexed_event_source_list* sources);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withSources(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, dxfg_indexed_event_source_list* sources);

//dxfg_indexed_tx_model_t* dxfg_IndexedTxModel_Builder_build(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source);
void* /*JavaObjectHandle<IndexedTxModel>*/ build(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withBatchProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, int32_t isBatchProcessing);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withBatchProcessing(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, bool isBatchProcessing);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, int32_t isSnapshotProcessing);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withSnapshotProcessing(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, bool isSnapshotProcessing);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withFeed(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, dxfg_feed_t* feed);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withFeed(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, const JavaObjectHandle<DXFeed>& feed);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSymbol(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, dxfg_symbol_t* symbol);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withSymbol(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, const SymbolWrapper& symbol);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withListener(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withListener(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, void* /*const JavaOjectHandle<TxModelListener>&*/ listener);

//dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withExecutor(graal_isolatethread_t* thread, dxfg_indexed_tx_model_builder_t* source, dxfg_executor_t* executor);
void* /*JavaObjectHandle<IndexedTxModel::Builder>*/ withExecutor(void* /*const JavaObjectHandle<IndexedTxModel::Builder>&*/ source, void* executor);

}
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()