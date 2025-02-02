// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../model/TimeSeriesTxModel.hpp"
#include "../../model/TxModelListener.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

class EventTypeEnum;
class IndexedEventSource;
struct DXFeed;
struct SymbolWrapper;

namespace isolated::model::IsolatedTimeSeriesTxModel {

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_newBuilder(graal_isolatethread_t* thread,
// dxfg_event_clazz_t eventType);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType);

// int64_t dxfg_TimeSeriesTxModel_getFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
std::int64_t getFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_TimeSeriesTxModel_setFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// int64_t fromTime);
void setFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model, std::int64_t fromTime);

// int32_t dxfg_TimeSeriesTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_TimeSeriesTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t*
// source);
bool isSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

// int32_t dxfg_TimeSeriesTxModel_attach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void attach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_TimeSeriesTxModel_detach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void detach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_TimeSeriesTxModel_close(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
void close(const JavaObjectHandle<TimeSeriesTxModelTag> &model);

namespace Builder {

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFromTime(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, int64_t fromTime);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFromTime(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          std::int64_t fromTime);

// dxfg_time_series_tx_model_t* dxfg_TimeSeriesTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source);
JavaObjectHandle<TimeSeriesTxModelTag> build(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withBatchProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isBatchProcessing);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isSnapshotProcessing);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFeed(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                      const JavaObjectHandle<DXFeed> &feed);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withSymbol(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                        const SymbolWrapper &symbol);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withListener(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          const JavaObjectHandle<TxModelListenerTag> &listener);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withExecutor(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          void *executor);

} // namespace Builder
} // namespace isolated::model::IsolatedTimeSeriesTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()