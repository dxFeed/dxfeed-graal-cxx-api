// Copyright (c) 2024 Devexperts LLC.
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
JavaObjectHandle<TimeSeriesTxModel::Builder> newBuilder(const EventTypeEnum &eventType);

// int64_t dxfg_TimeSeriesTxModel_getFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
std::int64_t getFromTime(const JavaObjectHandle<TimeSeriesTxModel> &model);

// int32_t dxfg_TimeSeriesTxModel_setFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// int64_t fromTime);
void setFromTime(const JavaObjectHandle<TimeSeriesTxModel> &model, std::int64_t fromTime);

// int32_t dxfg_TimeSeriesTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<TimeSeriesTxModel> &model);

// int32_t dxfg_TimeSeriesTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t*
// source);
bool isSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModel> &model);

// int32_t dxfg_TimeSeriesTxModel_attach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void attach(const JavaObjectHandle<TimeSeriesTxModel> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_TimeSeriesTxModel_detach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void detach(const JavaObjectHandle<TimeSeriesTxModel> &model, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_TimeSeriesTxModel_close(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
void close(const JavaObjectHandle<TimeSeriesTxModel> &model);

namespace Builder {

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFromTime(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, int64_t fromTime);
JavaObjectHandle<TimeSeriesTxModel::Builder> withFromTime(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder,
                                                          std::int64_t fromTime);

// dxfg_time_series_tx_model_t* dxfg_TimeSeriesTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source);
JavaObjectHandle<TimeSeriesTxModel> build(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withBatchProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<TimeSeriesTxModel::Builder>
withBatchProcessing(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder, bool isBatchProcessing);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<TimeSeriesTxModel::Builder>
withSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder, bool isSnapshotProcessing);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<TimeSeriesTxModel::Builder> withFeed(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder,
                                                      const JavaObjectHandle<DXFeed> &feed);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<TimeSeriesTxModel::Builder> withSymbol(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder,
                                                        const SymbolWrapper &symbol);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<TimeSeriesTxModel::Builder> withListener(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder,
                                                          const JavaObjectHandle<TxModelListener> &listener);

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<TimeSeriesTxModel::Builder> withExecutor(const JavaObjectHandle<TimeSeriesTxModel::Builder> &builder,
                                                          void *executor);

} // namespace Builder
} // namespace isolated::model::IsolatedTimeSeriesTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()