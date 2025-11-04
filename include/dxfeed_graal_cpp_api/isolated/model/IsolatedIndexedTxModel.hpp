// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../model/IndexedTxModel.hpp"
#include "../../model/TxModelListener.hpp"

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

namespace isolated::model::IsolatedIndexedTxModel {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_newBuilder(graal_isolatethread_t* thread, dxfg_event_clazz_t
// eventType);
JavaObjectHandle<IndexedTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType);

// dxfg_indexed_event_source_list* dxfg_IndexedTxModel_getSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_t* source);
void *getSources(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_IndexedTxModel_setSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source,
// dxfg_indexed_event_source_list* sources);
void setSources(const JavaObjectHandle<IndexedTxModelTag> &model, void *sources);

// int32_t dxfg_IndexedTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_IndexedTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isSnapshotProcessing(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_IndexedTxModel_attach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void attach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_IndexedTxModel_detach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void detach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_IndexedTxModel_close(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
void close(const JavaObjectHandle<IndexedTxModelTag> &model);

namespace Builder {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_indexed_event_source_list* sources);
JavaObjectHandle<IndexedTxModelBuilderTag> withSources(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                       void *sources);

// dxfg_indexed_tx_model_t* dxfg_IndexedTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source);
JavaObjectHandle<IndexedTxModelTag> build(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withBatchProcessing(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<IndexedTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isBatchProcessing);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<IndexedTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isSnapshotProcessing);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<IndexedTxModelBuilderTag> withFeed(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                    const JavaObjectHandle<DXFeed> &feed);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<IndexedTxModelBuilderTag> withSymbol(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                      const SymbolWrapper &symbol);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<IndexedTxModelBuilderTag> withListener(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        const JavaObjectHandle<TxModelListenerTag> &listener);

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<IndexedTxModelBuilderTag> withExecutor(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        void *executor);

} // namespace Builder
} // namespace isolated::model::IsolatedIndexedTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()