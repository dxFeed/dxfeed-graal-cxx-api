// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/model/IndexedTxModel.hpp"
#include "../../../include/dxfeed_graal_cpp_api/model/TxModelListener.hpp"

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

// The dxfg_v4_IndexedTxModel_ functions are experimental: newBuilder() checks the advanced revision of the SDK library
// first (Isolate::checkAdvancedRevision()).
namespace isolated::model::IsolatedIndexedTxModel {

// int32_t dxfg_v4_IndexedTxModel_newBuilder(graal_isolatethread_t *thread, dxfg_v4_event_clazz_t eventType,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType);

// int32_t dxfg_v4_IndexedTxModel_getSources(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source,
// DXFG_OUT dxfg_v4_indexed_event_source_list **sources);
void * /* dxfg_v4_indexed_event_source_list* */ getSources(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_v4_IndexedTxModel_setSources(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source,
// dxfg_v4_indexed_event_source_list *sources);
void setSources(const JavaObjectHandle<IndexedTxModelTag> &model,
                void * /* dxfg_v4_indexed_event_source_list* */ sources);

// int32_t dxfg_v4_IndexedTxModel_isBatchProcessing(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source,
// DXFG_OUT int32_t *batchProcessing);
bool isBatchProcessing(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_v4_IndexedTxModel_isSnapshotProcessing(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_t *source, DXFG_OUT int32_t *snapshotProcessing);
bool isSnapshotProcessing(const JavaObjectHandle<IndexedTxModelTag> &model);

// int32_t dxfg_v4_IndexedTxModel_attach(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source,
// dxfg_v4_feed_t *feed);
void attach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_IndexedTxModel_detach(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source,
// dxfg_v4_feed_t *feed);
void detach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_IndexedTxModel_close(graal_isolatethread_t *thread, dxfg_v4_indexed_tx_model_t *source);
void close(const JavaObjectHandle<IndexedTxModelTag> &model);

// Each dxfg_v4_IndexedTxModel_Builder_with* function writes a new builder to its out parameter.
namespace Builder {

// int32_t dxfg_v4_IndexedTxModel_Builder_withSources(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, dxfg_v4_indexed_event_source_list *sources,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> withSources(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                       void * /* dxfg_v4_indexed_event_source_list* */ sources);

// int32_t dxfg_v4_IndexedTxModel_Builder_build(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, DXFG_OUT dxfg_v4_indexed_tx_model_t **indexedTxModel);
JavaObjectHandle<IndexedTxModelTag> build(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder);

// int32_t dxfg_v4_IndexedTxModel_Builder_withBatchProcessing(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, int32_t isBatchProcessing,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isBatchProcessing);

// int32_t dxfg_v4_IndexedTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, int32_t isSnapshotProcessing,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isSnapshotProcessing);

// int32_t dxfg_v4_IndexedTxModel_Builder_withFeed(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, dxfg_v4_feed_t *feed,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> withFeed(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                    const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_IndexedTxModel_Builder_withSymbol(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, dxfg_v4_symbol_t *symbol,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> withSymbol(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                      const SymbolWrapper &symbol);

// int32_t dxfg_v4_IndexedTxModel_Builder_withListener(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, dxfg_v4_tx_model_listener_t *listener,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> withListener(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        const JavaObjectHandle<TxModelListenerTag> &listener);

// int32_t dxfg_v4_IndexedTxModel_Builder_withExecutor(graal_isolatethread_t *thread,
// dxfg_v4_indexed_tx_model_builder_t *source, dxfg_v4_executor_t *executor,
// DXFG_OUT dxfg_v4_indexed_tx_model_builder_t **indexedTxModelBuilder);
JavaObjectHandle<IndexedTxModelBuilderTag> withExecutor(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        void * /* dxfg_v4_executor_t* */ executor);

} // namespace Builder
} // namespace isolated::model::IsolatedIndexedTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
