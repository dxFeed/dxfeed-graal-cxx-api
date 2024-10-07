// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

//#include "../../model/TimeSeriesTxModel.hpp"

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

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_newBuilder(graal_isolatethread_t* thread, dxfg_event_clazz_t eventType);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ newBuilder(const EventTypeEnum &eventType);

//int64_t dxfg_TimeSeriesTxModel_getFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
std::int64_t getFromTime(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source);

//int32_t dxfg_TimeSeriesTxModel_setFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source, int64_t fromTime);
void setFromTime(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source, std::int64_t fromTime);

//int32_t dxfg_TimeSeriesTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
bool isBatchProcessing(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source);

//int32_t dxfg_TimeSeriesTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
bool isSnapshotProcessing(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source);

//int32_t dxfg_TimeSeriesTxModel_attach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source, dxfg_feed_t* feed);
void attach(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source, const JavaObjectHandle<DXFeed>& feed);

//int32_t dxfg_TimeSeriesTxModel_detach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source, dxfg_feed_t* feed);
void detach(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source, const JavaObjectHandle<DXFeed>& feed);

//int32_t dxfg_TimeSeriesTxModel_close(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
void close(void* /* const JavaObjectHandle<TimeSeriesTxModel>& */ source);

namespace Builder {

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, int64_t fromTime);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withFromTime(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, std::int64_t fromTime);

//dxfg_time_series_tx_model_t* dxfg_TimeSeriesTxModel_Builder_build(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source);
void* /*JavaObjectHandle<TimeSeriesTxModel>*/ build(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withBatchProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, int32_t isBatchProcessing);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withBatchProcessing(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, bool isBatchProcessing);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, int32_t isSnapshotProcessing);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withSnapshotProcessing(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, bool isSnapshotProcessing);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFeed(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, dxfg_feed_t* feed);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withFeed(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, const JavaObjectHandle<DXFeed>& feed);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSymbol(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, dxfg_symbol_t* symbol);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withSymbol(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, const SymbolWrapper& symbol);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withListener(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withListener(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, void* /*const JavaOjectHandle<TxModelListener>&*/ listener);

//dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withExecutor(graal_isolatethread_t* thread, dxfg_time_series_tx_model_builder_t* source, dxfg_executor_t* executor);
void* /*JavaObjectHandle<TimeSeriesTxModel::Builder>*/ withExecutor(void* /*const JavaObjectHandle<TimeSeriesTxModel::Builder>&*/ source, void* executor);

}
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()