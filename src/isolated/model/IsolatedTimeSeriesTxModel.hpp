// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/model/TimeSeriesTxModel.hpp"
#include "../../../include/dxfeed_graal_cpp_api/model/TxModelListener.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

// The dxfg_v4_TimeSeriesTxModel_ functions are experimental: newBuilder() checks the advanced revision of the SDK
// library first (Isolate::checkAdvancedRevision()).
namespace isolated::model::IsolatedTimeSeriesTxModel {

// int32_t dxfg_v4_TimeSeriesTxModel_newBuilder(graal_isolatethread_t *thread, dxfg_v4_event_clazz_t eventType,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType);

// int32_t dxfg_v4_TimeSeriesTxModel_getFromTime(graal_isolatethread_t *thread, dxfg_v4_time_series_tx_model_t *source,
// DXFG_OUT int64_t *fromTime);
std::int64_t getFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_v4_TimeSeriesTxModel_setFromTime(graal_isolatethread_t *thread, dxfg_v4_time_series_tx_model_t *source,
// int64_t fromTime);
void setFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model, std::int64_t fromTime);

// int32_t dxfg_v4_TimeSeriesTxModel_isBatchProcessing(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_t *source, DXFG_OUT int32_t *batchProcessing);
bool isBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_v4_TimeSeriesTxModel_isSnapshotProcessing(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_t *source, DXFG_OUT int32_t *snapshotProcessing);
bool isSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_v4_TimeSeriesTxModel_attach(graal_isolatethread_t *thread, dxfg_v4_time_series_tx_model_t *source,
// dxfg_v4_feed_t *feed);
void attach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_TimeSeriesTxModel_detach(graal_isolatethread_t *thread, dxfg_v4_time_series_tx_model_t *source,
// dxfg_v4_feed_t *feed);
void detach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_TimeSeriesTxModel_close(graal_isolatethread_t *thread, dxfg_v4_time_series_tx_model_t *source);
void close(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// Each dxfg_v4_TimeSeriesTxModel_Builder_with* function writes a new builder to its out parameter.
namespace Builder {

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withFromTime(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, int64_t fromTime,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFromTime(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           std::int64_t fromTime);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_build(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, DXFG_OUT dxfg_v4_time_series_tx_model_t **timeSeriesTxModel);
JavaObjectHandle<TimeSeriesTxModelTag> build(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withBatchProcessing(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, int32_t isBatchProcessing,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isBatchProcessing);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, int32_t isSnapshotProcessing,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isSnapshotProcessing);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withFeed(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, dxfg_v4_feed_t *feed,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFeed(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                       const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withSymbol(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, dxfg_v4_symbol_t *symbol,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withSymbol(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                         const SymbolWrapper &symbol);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withListener(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, dxfg_v4_tx_model_listener_t *listener,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withListener(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           const JavaObjectHandle<TxModelListenerTag> &listener);

// int32_t dxfg_v4_TimeSeriesTxModel_Builder_withExecutor(graal_isolatethread_t *thread,
// dxfg_v4_time_series_tx_model_builder_t *source, dxfg_v4_executor_t *executor,
// DXFG_OUT dxfg_v4_time_series_tx_model_builder_t **timeSeriesTxModelBuilder);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withExecutor(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           void * /* dxfg_v4_executor_t* */ executor);

} // namespace Builder
} // namespace isolated::model::IsolatedTimeSeriesTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
