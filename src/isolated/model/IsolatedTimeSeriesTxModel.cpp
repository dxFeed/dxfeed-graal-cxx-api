// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedTimeSeriesTxModel.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTimeSeriesTxModel {

namespace {

dxfg_v4_time_series_tx_model_t *toGraal(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const char *function) {
    if (!model) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `model` handle is invalid");
    }

    return static_cast<dxfg_v4_time_series_tx_model_t *>(model.get());
}

dxfg_v4_time_series_tx_model_builder_t *toGraal(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                const char *function) {
    if (!builder) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `builder` handle is invalid");
    }

    return static_cast<dxfg_v4_time_series_tx_model_builder_t *>(builder.get());
}

dxfg_v4_feed_t *toGraal(const JavaObjectHandle<DXFeed> &feed, const char *function) {
    if (!feed) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `feed` handle is invalid");
    }

    return static_cast<dxfg_v4_feed_t *>(feed.get());
}

} // namespace

JavaObjectHandle<TimeSeriesTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeSeriesTxModel_newBuilder, static_cast<dxfg_v4_event_clazz_t>(eventType.getId()))};
}

std::int64_t getFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_TimeSeriesTxModel_getFromTime,
                                        toGraal(model, "dxfg_v4_TimeSeriesTxModel_getFromTime"));
}

void setFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model, std::int64_t fromTime) {
    runGraalV4Function(dxfg_v4_TimeSeriesTxModel_setFromTime, toGraal(model, "dxfg_v4_TimeSeriesTxModel_setFromTime"),
                       fromTime);
}

bool isBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_TimeSeriesTxModel_isBatchProcessing,
                                        toGraal(model, "dxfg_v4_TimeSeriesTxModel_isBatchProcessing")) == 1;
}

bool isSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_TimeSeriesTxModel_isSnapshotProcessing,
                                        toGraal(model, "dxfg_v4_TimeSeriesTxModel_isSnapshotProcessing")) == 1;
}

void attach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    runGraalV4Function(dxfg_v4_TimeSeriesTxModel_attach, toGraal(model, "dxfg_v4_TimeSeriesTxModel_attach"),
                       toGraal(feed, "dxfg_v4_TimeSeriesTxModel_attach"));
}

void detach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    runGraalV4Function(dxfg_v4_TimeSeriesTxModel_detach, toGraal(model, "dxfg_v4_TimeSeriesTxModel_detach"),
                       toGraal(feed, "dxfg_v4_TimeSeriesTxModel_detach"));
}

void close(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    runGraalV4Function(dxfg_v4_TimeSeriesTxModel_close, toGraal(model, "dxfg_v4_TimeSeriesTxModel_close"));
}

namespace Builder {

JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFromTime(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           std::int64_t fromTime) {
    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeSeriesTxModel_Builder_withFromTime,
        toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withFromTime"), fromTime)};
}

JavaObjectHandle<TimeSeriesTxModelTag> build(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder) {
    return JavaObjectHandle<TimeSeriesTxModelTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeSeriesTxModel_Builder_build, toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_build"))};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isBatchProcessing) {
    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeSeriesTxModel_Builder_withBatchProcessing,
        toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withBatchProcessing"), isBatchProcessing ? 1 : 0)};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isSnapshotProcessing) {
    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_TimeSeriesTxModel_Builder_withSnapshotProcessing,
        toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withSnapshotProcessing"), isSnapshotProcessing ? 1 : 0)};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFeed(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                       const JavaObjectHandle<DXFeed> &feed) {
    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeSeriesTxModel_Builder_withFeed,
                                            toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withFeed"),
                                            toGraal(feed, "dxfg_v4_TimeSeriesTxModel_Builder_withFeed"))};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag> withSymbol(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                         const SymbolWrapper &symbol) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withSymbol");
    const auto graalSymbol = symbol.toGraalUnique();

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeSeriesTxModel_Builder_withSymbol, graalBuilder,
                                            static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()))};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag> withListener(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           const JavaObjectHandle<TxModelListenerTag> &listener) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_TimeSeriesTxModel_Builder_withListener`. "
                                       "The `listener` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeSeriesTxModel_Builder_withListener, graalBuilder,
                                            static_cast<dxfg_v4_tx_model_listener_t *>(listener.get()))};
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag> withExecutor(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                           void *executor) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_TimeSeriesTxModel_Builder_withExecutor");

    if (!executor) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_TimeSeriesTxModel_Builder_withExecutor`. "
                                       "The `executor` is nullptr");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_TimeSeriesTxModel_Builder_withExecutor, graalBuilder,
                                            static_cast<dxfg_v4_executor_t *>(executor))};
}

} // namespace Builder
} // namespace isolated::model::IsolatedTimeSeriesTxModel

DXFCPP_END_NAMESPACE
