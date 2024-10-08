// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../model/IndexedTxModel.hpp"
#include "../../model/TxModelListener.hpp"

#include <cstdint>
#include <type_traits>
#include <unordered_set>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

namespace isolated::model::IsolatedIndexedTxModel {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_newBuilder(graal_isolatethread_t* thread, dxfg_event_clazz_t
// eventType);
JavaObjectHandle<IndexedTxModel::Builder> newBuilder(const EventTypeEnum &eventType);

// dxfg_indexed_event_source_list* dxfg_IndexedTxModel_getSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_t* source);
void* getSources(const JavaObjectHandle<IndexedTxModel> &model);

// int32_t dxfg_IndexedTxModel_setSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source,
// dxfg_indexed_event_source_list* sources);
void setSources(const JavaObjectHandle<IndexedTxModel> &model, void *sources);

// int32_t dxfg_IndexedTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<IndexedTxModel> &model);

// int32_t dxfg_IndexedTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isSnapshotProcessing(const JavaObjectHandle<IndexedTxModel> &model);

// int32_t dxfg_IndexedTxModel_attach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void attach(const JavaObjectHandle<IndexedTxModel> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_IndexedTxModel_detach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void detach(const JavaObjectHandle<IndexedTxModel> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_IndexedTxModel_close(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
void close(const JavaObjectHandle<IndexedTxModel> &model);

namespace Builder {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_indexed_event_source_list* sources);
JavaObjectHandle<IndexedTxModel::Builder> withSources(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                      void *sources);

// dxfg_indexed_tx_model_t* dxfg_IndexedTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source);
JavaObjectHandle<IndexedTxModel> build(const JavaObjectHandle<IndexedTxModel::Builder> &builder);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withBatchProcessing(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<IndexedTxModel::Builder> withBatchProcessing(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                              bool isBatchProcessing);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<IndexedTxModel::Builder>
withSnapshotProcessing(const JavaObjectHandle<IndexedTxModel::Builder> &builder, bool isSnapshotProcessing);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<IndexedTxModel::Builder> withFeed(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                   const JavaObjectHandle<DXFeed> &feed);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<IndexedTxModel::Builder> withSymbol(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                     const SymbolWrapper &symbol);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<IndexedTxModel::Builder> withListener(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                       const JavaObjectHandle<TxModelListener> &listener);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<IndexedTxModel::Builder> withExecutor(const JavaObjectHandle<IndexedTxModel::Builder> &builder,
                                                       void *executor);

} // namespace Builder
} // namespace isolated::model::IsolatedIndexedTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()