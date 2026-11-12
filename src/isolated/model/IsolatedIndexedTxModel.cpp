// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedIndexedTxModel.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedIndexedTxModel {

namespace {

dxfg_v4_indexed_tx_model_t *toGraal(const JavaObjectHandle<IndexedTxModelTag> &model, const char *function) {
    if (!model) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `model` handle is invalid");
    }

    return static_cast<dxfg_v4_indexed_tx_model_t *>(model.get());
}

dxfg_v4_indexed_tx_model_builder_t *toGraal(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                            const char *function) {
    if (!builder) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `builder` handle is invalid");
    }

    return static_cast<dxfg_v4_indexed_tx_model_builder_t *>(builder.get());
}

dxfg_v4_feed_t *toGraal(const JavaObjectHandle<DXFeed> &feed, const char *function) {
    if (!feed) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `feed` handle is invalid");
    }

    return static_cast<dxfg_v4_feed_t *>(feed.get());
}

} // namespace

JavaObjectHandle<IndexedTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedTxModel_newBuilder, static_cast<dxfg_v4_event_clazz_t>(eventType.getId()))};
}

void *getSources(const JavaObjectHandle<IndexedTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_IndexedTxModel_getSources,
                                        toGraal(model, "dxfg_v4_IndexedTxModel_getSources"));
}

void setSources(const JavaObjectHandle<IndexedTxModelTag> &model, void *sources) {
    auto *graalModel = toGraal(model, "dxfg_v4_IndexedTxModel_setSources");

    if (!sources) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_IndexedTxModel_setSources`. The "
                                       "`sources` is nullptr");
    }

    runGraalV4Function(dxfg_v4_IndexedTxModel_setSources, graalModel,
                       static_cast<dxfg_v4_indexed_event_source_list *>(sources));
}

bool isBatchProcessing(const JavaObjectHandle<IndexedTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_IndexedTxModel_isBatchProcessing,
                                        toGraal(model, "dxfg_v4_IndexedTxModel_isBatchProcessing")) == 1;
}

bool isSnapshotProcessing(const JavaObjectHandle<IndexedTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_IndexedTxModel_isSnapshotProcessing,
                                        toGraal(model, "dxfg_v4_IndexedTxModel_isSnapshotProcessing")) == 1;
}

void attach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    runGraalV4Function(dxfg_v4_IndexedTxModel_attach, toGraal(model, "dxfg_v4_IndexedTxModel_attach"),
                       toGraal(feed, "dxfg_v4_IndexedTxModel_attach"));
}

void detach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    runGraalV4Function(dxfg_v4_IndexedTxModel_detach, toGraal(model, "dxfg_v4_IndexedTxModel_detach"),
                       toGraal(feed, "dxfg_v4_IndexedTxModel_detach"));
}

void close(const JavaObjectHandle<IndexedTxModelTag> &model) {
    runGraalV4Function(dxfg_v4_IndexedTxModel_close, toGraal(model, "dxfg_v4_IndexedTxModel_close"));
}

namespace Builder {

JavaObjectHandle<IndexedTxModelBuilderTag> withSources(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                       void *sources) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withSources");

    if (!sources) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_IndexedTxModel_Builder_withSources`. The "
                                       "`sources` is nullptr");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_IndexedTxModel_Builder_withSources, graalBuilder,
                                            static_cast<dxfg_v4_indexed_event_source_list *>(sources))};
}

JavaObjectHandle<IndexedTxModelTag> build(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder) {
    return JavaObjectHandle<IndexedTxModelTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedTxModel_Builder_build, toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_build"))};
}

JavaObjectHandle<IndexedTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isBatchProcessing) {
    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedTxModel_Builder_withBatchProcessing,
        toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withBatchProcessing"), isBatchProcessing ? 1 : 0)};
}

JavaObjectHandle<IndexedTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isSnapshotProcessing) {
    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedTxModel_Builder_withSnapshotProcessing,
        toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withSnapshotProcessing"), isSnapshotProcessing ? 1 : 0)};
}

JavaObjectHandle<IndexedTxModelBuilderTag> withFeed(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                    const JavaObjectHandle<DXFeed> &feed) {
    return JavaObjectHandle<IndexedTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_IndexedTxModel_Builder_withFeed,
                                            toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withFeed"),
                                            toGraal(feed, "dxfg_v4_IndexedTxModel_Builder_withFeed"))};
}

JavaObjectHandle<IndexedTxModelBuilderTag> withSymbol(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                      const SymbolWrapper &symbol) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withSymbol");
    const auto graalSymbol = symbol.toGraalUnique();

    return JavaObjectHandle<IndexedTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_IndexedTxModel_Builder_withSymbol, graalBuilder,
                                            static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()))};
}

JavaObjectHandle<IndexedTxModelBuilderTag> withListener(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        const JavaObjectHandle<TxModelListenerTag> &listener) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_IndexedTxModel_Builder_withListener`. The "
                                       "`listener` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_IndexedTxModel_Builder_withListener, graalBuilder,
                                            static_cast<dxfg_v4_tx_model_listener_t *>(listener.get()))};
}

JavaObjectHandle<IndexedTxModelBuilderTag> withExecutor(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                        void *executor) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedTxModel_Builder_withExecutor");

    if (!executor) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_IndexedTxModel_Builder_withExecutor`. The "
                                       "`executor` is nullptr");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_IndexedTxModel_Builder_withExecutor, graalBuilder,
                                            static_cast<dxfg_v4_executor_t *>(executor))};
}

} // namespace Builder
} // namespace isolated::model::IsolatedIndexedTxModel

DXFCPP_END_NAMESPACE
