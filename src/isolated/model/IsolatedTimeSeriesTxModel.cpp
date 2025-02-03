// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedTimeSeriesTxModel.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedTimeSeriesTxModel {

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_newBuilder(graal_isolatethread_t* thread,
// dxfg_event_clazz_t eventType);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_newBuilder, static_cast<dxfg_event_clazz_t>(eventType.getId()))};
}

// int64_t dxfg_TimeSeriesTxModel_getFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
std::int64_t getFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_getFromTime`. The "
                                       "`model` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_getFromTime,
                                              static_cast<dxfg_time_series_tx_model_t *>(model.get()));
}

// int32_t dxfg_TimeSeriesTxModel_setFromTime(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// int64_t fromTime);
void setFromTime(const JavaObjectHandle<TimeSeriesTxModelTag> &model, std::int64_t fromTime) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_setFromTime`. The "
                                       "`model` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_setFromTime,
                                       static_cast<dxfg_time_series_tx_model_t *>(model.get()), fromTime);
}

// int32_t dxfg_TimeSeriesTxModel_isBatchProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
bool isBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_isBatchProcessing`. The "
                                       "`model` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_isBatchProcessing,
                                              static_cast<dxfg_time_series_tx_model_t *>(model.get())) == 1;
}

// int32_t dxfg_TimeSeriesTxModel_isSnapshotProcessing(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t*
// source);
bool isSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_isSnapshotProcessing`. The "
                                       "`model` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_isSnapshotProcessing,
                                              static_cast<dxfg_time_series_tx_model_t *>(model.get())) == 1;
}

// int32_t dxfg_TimeSeriesTxModel_attach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void attach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_attach`. The "
                                       "`model` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_attach`. The "
                                       "`feed` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_attach,
                                       static_cast<dxfg_time_series_tx_model_t *>(model.get()),
                                       static_cast<dxfg_feed_t *>(feed.get()));
}

// int32_t dxfg_TimeSeriesTxModel_detach(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source,
// dxfg_feed_t* feed);
void detach(const JavaObjectHandle<TimeSeriesTxModelTag> &model, const JavaObjectHandle<DXFeed> &feed) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_detach`. The "
                                       "`model` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_detach`. The "
                                       "`feed` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_detach,
                                       static_cast<dxfg_time_series_tx_model_t *>(model.get()),
                                       static_cast<dxfg_feed_t *>(feed.get()));
}

// int32_t dxfg_TimeSeriesTxModel_close(graal_isolatethread_t* thread, dxfg_time_series_tx_model_t* source);
void close(const JavaObjectHandle<TimeSeriesTxModelTag> &model) {
    if (!model) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_close`. The "
                                       "`model` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_TimeSeriesTxModel_close,
                                       static_cast<dxfg_time_series_tx_model_t *>(model.get()));
}

namespace Builder {

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFromTime(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, int64_t fromTime);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFromTime(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          std::int64_t fromTime) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withFromTime`. The "
                                       "`builder` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{
        runGraalFunctionAndThrowIfNullptr(dxfg_TimeSeriesTxModel_Builder_withFromTime,
                                          static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()), fromTime)};
}

// dxfg_time_series_tx_model_t* dxfg_TimeSeriesTxModel_Builder_build(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source);
JavaObjectHandle<TimeSeriesTxModelTag> build(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_build`. The "
                                       "`builder` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_build, static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()))};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withBatchProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isBatchProcessing);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withBatchProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isBatchProcessing) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withBatchProcessing`. The "
            "`builder` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withBatchProcessing,
        static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()), isBatchProcessing ? 1 : 0)};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing(graal_isolatethread_t*
// thread, dxfg_time_series_tx_model_builder_t* source, int32_t isSnapshotProcessing);
JavaObjectHandle<TimeSeriesTxModelBuilderTag>
withSnapshotProcessing(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder, bool isSnapshotProcessing) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing`. The "
            "`builder` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withSnapshotProcessing,
        static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()), isSnapshotProcessing ? 1 : 0)};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withFeed(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_feed_t* feed);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withFeed(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                      const JavaObjectHandle<DXFeed> &feed) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withFeed`. The "
                                       "`builder` handle is invalid");
    }

    if (!feed) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withFeed`. The "
                                       "`feed` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withFeed, static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_feed_t *>(feed.get()))};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withSymbol(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_symbol_t* symbol);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withSymbol(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                        const SymbolWrapper &symbol) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withSymbol`. The "
                                       "`builder` handle is invalid");
    }

    auto graalSymbol = symbol.toGraalUnique();

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withSymbol, static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_symbol_t *>(graalSymbol.get()))};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withListener(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_tx_model_listener_t* listener);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withListener(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          const JavaObjectHandle<TxModelListenerTag> &listener) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withListener`. The "
                                       "`builder` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withListener`. The "
                                       "`listener` handle is invalid");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withListener, static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_tx_model_listener_t *>(listener.get()))};
}

// dxfg_time_series_tx_model_builder_t* dxfg_TimeSeriesTxModel_Builder_withExecutor(graal_isolatethread_t* thread,
// dxfg_time_series_tx_model_builder_t* source, dxfg_executor_t* executor);
JavaObjectHandle<TimeSeriesTxModelBuilderTag> withExecutor(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &builder,
                                                          void *executor) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withExecutor`. The "
                                       "`builder` handle is invalid");
    }

    if (!executor) {
        throw InvalidArgumentException("Unable to execute function `dxfg_TimeSeriesTxModel_Builder_withExecutor`. The "
                                       "`executor` is null");
    }

    return JavaObjectHandle<TimeSeriesTxModelBuilderTag>{runGraalFunctionAndThrowIfNullptr(
        dxfg_TimeSeriesTxModel_Builder_withExecutor, static_cast<dxfg_time_series_tx_model_builder_t *>(builder.get()),
        static_cast<dxfg_executor_t *>(executor))};
}

} // namespace Builder

} // namespace isolated::model::IsolatedTimeSeriesTxModel

DXFCPP_END_NAMESPACE