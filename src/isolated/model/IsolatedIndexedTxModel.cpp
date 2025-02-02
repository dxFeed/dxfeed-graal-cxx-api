// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedIndexedTxModel {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_newBuilder(graal_isolatethread_t* thread, dxfg_event_clazz_t
// eventType);
JavaObjectHandle<IndexedTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_newBuilder, static_cast<dxfg_event_clazz_t>(eventType.getId()))};
}

// dxfg_indexed_event_source_list* dxfg_IndexedTxModel_getSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_t* source);
void *getSources(const JavaObjectHandle<IndexedTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_getSources`. The "
                                       "`model` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_getSources, static_cast<dxfg_indexed_tx_model_t *>(model.get())));
}

// int32_t dxfg_IndexedTxModel_setSources(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source,
// dxfg_indexed_event_source_list* sources);
void setSources(const JavaObjectHandle<IndexedTxModelTag> &model, void *sources) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_setSources`. The "
                                       "`model` handle is invalid");
    }

    if (!sources) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_setSources`. The "
                                       "`sources` is nullptr");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_setSources,
                                       static_cast<dxfg_indexed_tx_model_t *>(model.get()),
                                       static_cast<dxfg_indexed_event_source_list *>(sources));
}

// int32_t dxfg_IndexedTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<IndexedTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_isBatchProcessing`. The "
                                       "`model` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_isBatchProcessing,
                                              static_cast<dxfg_indexed_tx_model_t *>(model.get())) == 1;
}

// int32_t dxfg_IndexedTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
bool isSnapshotProcessing(const JavaObjectHandle<IndexedTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_isSnapshotProcessing`. The "
                                       "`model` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_isSnapshotProcessing,
                                              static_cast<dxfg_indexed_tx_model_t *>(model.get())) == 1;
}

// int32_t dxfg_IndexedTxModel_attach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void attach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_attach`. The "
                                       "`model` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_attach`. The "
                                       "`feed` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_attach, static_cast<dxfg_indexed_tx_model_t *>(model.get()),
                                       static_cast<dxfg_feed_t *>(feed.get()));
}

// int32_t dxfg_IndexedTxModel_detach(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source, dxfg_feed_t*
// feed);
void detach(const JavaObjectHandle<IndexedTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_detach`. The "
                                       "`model` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_detach`. The "
                                       "`feed` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_detach, static_cast<dxfg_indexed_tx_model_t *>(model.get()),
                                       static_cast<dxfg_feed_t *>(feed.get()));
}

// int32_t dxfg_IndexedTxModel_close(graal_isolatethread_t* thread, dxfg_indexed_tx_model_t* source);
void close(const JavaObjectHandle<IndexedTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_close`. The "
                                       "`model` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_IndexedTxModel_close, static_cast<dxfg_indexed_tx_model_t *>(model.get()));
}

namespace Builder {

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSources(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_indexed_event_source_list* sources);
JavaObjectHandle<IndexedTxModelBuilderTag> withSources(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                      void *sources) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withSources`. The "
                                       "`builder` handle is invalid");
    }

    if (!sources) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withSources`. The "
                                       "`sources` is nullptr");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withSources, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_indexed_event_source_list *>(sources))};
}

// dxfg_indexed_tx_model_t* dxfg_IndexedTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source);
JavaObjectHandle<IndexedTxModelTag> build(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_build`. The "
                                       "`builder` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_build, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()))};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withBatchProcessing(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<IndexedTxModelBuilderTag> withBatchProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                              bool isBatchProcessing) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_IndexedTxModel_Builder_withBatchProcessing`. The "
            "`builder` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withBatchProcessing, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        isBatchProcessing ? 1 : 0)};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t*
// thread, dxfg_indexed_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<IndexedTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder, bool isSnapshotProcessing) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_IndexedTxModel_Builder_withSnapshotProcessing`. The "
            "`builder` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withSnapshotProcessing,
        static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()), isSnapshotProcessing ? 1 : 0)};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<IndexedTxModelBuilderTag> withFeed(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                   const JavaObjectHandle<DXFeed> &feed) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withFeed`. The "
                                       "`builder` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withFeed`. The "
                                       "`feed` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withFeed, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_feed_t *>(feed.get()))};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<IndexedTxModelBuilderTag> withSymbol(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                     const SymbolWrapper &symbol) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withSymbol`. The "
                                       "`builder` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withSymbol, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_symbol_t *>(graalSymbol.get()))};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<IndexedTxModelBuilderTag> withListener(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                       const JavaObjectHandle<TxModelListenerTag> &listener) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withListener`. The "
                                       "`builder` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withListener`. The "
                                       "`listener` handle is invalid");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withListener, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_tx_model_listener_t *>(listener.get()))};
}

// dxfg_indexed_tx_model_builder_t* dxfg_IndexedTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_indexed_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<IndexedTxModelBuilderTag> withExecutor(const JavaObjectHandle<IndexedTxModelBuilderTag> &builder,
                                                       void *executor) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withExecutor`. The "
                                       "`builder` handle is invalid");
    }

    if (!executor) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedTxModel_Builder_withExecutor`. The "
                                       "`executor` is nullptr");
    }

    return JavaObjectHandle<IndexedTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_IndexedTxModel_Builder_withExecutor, static_cast<dxfg_indexed_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_executor_t *>(executor))};
}

} // namespace Builder
} // namespace isolated::model::IsolatedIndexedTxModel

DXFCPP_END_NAMESPACE