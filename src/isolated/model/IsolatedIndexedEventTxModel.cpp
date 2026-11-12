// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedIndexedEventTxModel.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/CallbackBarrier.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"
#include "../internal/IsolatedString.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedIndexedEventTxModel {

namespace {

dxfg_v4_indexed_event_tx_model_t *toGraal(const JavaObjectHandle<IndexedEventTxModelTag> &model,
                                          const char *function) {
    if (!model) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `model` handle is invalid");
    }

    return static_cast<dxfg_v4_indexed_event_tx_model_t *>(model.get());
}

dxfg_v4_indexed_event_tx_model_builder_t *toGraal(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                                  const char *function) {
    if (!builder) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `builder` handle is invalid");
    }

    return static_cast<dxfg_v4_indexed_event_tx_model_builder_t *>(builder.get());
}

// A deleter of std::unique_ptr: a failure to free the symbol must not escape it.
void releaseSymbol(dxfg_v4_symbol_t *symbol) noexcept {
    if (!symbol) {
        return;
    }

    runRelease("IndexedEventTxModel", [&] {
        runGraalV4Function(dxfg_v4_Symbol_release, symbol);
    });
}

// A deleter of std::unique_ptr: a failure to free the source must not escape it.
void releaseSource(dxfg_v4_indexed_event_source_t *source) noexcept {
    if (!source) {
        return;
    }

    runRelease("IndexedEventTxModel", [&] {
        event::IsolatedIndexedEventSource::release(source);
    });
}

} // namespace

JavaObjectHandle<IndexedEventTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<IndexedEventTxModelBuilderTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedEventTxModel_newBuilder, static_cast<dxfg_v4_event_clazz_t>(eventType.getId()))};
}

IndexedEventSubscriptionSymbol
getIndexedEventSubscriptionSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    auto *symbol = runGraalV4FunctionWithResult(
        dxfg_v4_IndexedEventTxModel_getIndexedEventSubscriptionSymbol,
        toGraal(model, "dxfg_v4_IndexedEventTxModel_getIndexedEventSubscriptionSymbol"));

    const std::unique_ptr<dxfg_v4_symbol_t, decltype(&releaseSymbol)> u{symbol, releaseSymbol};

    auto result = SymbolWrapper::fromGraal(symbol).asIndexedEventSubscriptionSymbol();

    if (!result) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_IndexedEventTxModel_getIndexedEventSubscriptionSymbol`. The result "
                                       "is not an IndexedEventSubscriptionSymbol");
    }

    return std::move(*result);
}

std::string getSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    const char *symbol = runGraalV4FunctionWithResult(dxfg_v4_IndexedEventTxModel_getSymbol,
                                                      toGraal(model, "dxfg_v4_IndexedEventTxModel_getSymbol"));

    const auto u = internal::IsolatedString::toUnique(symbol);

    return symbol ? std::string(symbol) : std::string();
}

EventSourceWrapper getSource(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    auto *source = runGraalV4FunctionWithResult(dxfg_v4_IndexedEventTxModel_getSource,
                                                toGraal(model, "dxfg_v4_IndexedEventTxModel_getSource"));

    const std::unique_ptr<dxfg_v4_indexed_event_source_t, decltype(&releaseSource)> u{source, releaseSource};

    return EventSourceWrapper::fromGraal(source);
}

JavaObjectHandle<SubscriptionController>
getSubscriptionController(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return JavaObjectHandle<SubscriptionController>{
        runGraalV4FunctionWithResult(dxfg_v4_IndexedEventTxModel_getSubscriptionController,
                                     toGraal(model, "dxfg_v4_IndexedEventTxModel_getSubscriptionController"))};
}

bool isClosed(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return runGraalV4FunctionWithResult(dxfg_v4_IndexedEventTxModel_isClosed,
                                        toGraal(model, "dxfg_v4_IndexedEventTxModel_isClosed")) == 1;
}

void close(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    runGraalV4Function(dxfg_v4_IndexedEventTxModel_close, toGraal(model, "dxfg_v4_IndexedEventTxModel_close"));
}

// The dxfg_v4_IndexedEventTxModel_Builder_with* functions change the builder itself.
namespace Builder {

void withFeed(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const JavaObjectHandle<DXFeed> &feed) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withFeed");

    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_IndexedEventTxModel_Builder_withFeed`. The `feed` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withFeed, graalBuilder,
                       static_cast<dxfg_v4_feed_t *>(feed.get()));
}

void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                const IndexedEventSubscriptionSymbol &symbol) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withSymbol");
    const auto graalSymbol = SymbolWrapper(symbol).toGraalUnique();

    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withSymbol, graalBuilder,
                       static_cast<dxfg_v4_symbol_t *>(graalSymbol.get()));
}

void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, std::string_view symbol) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withStringSymbol");
    const SdkString graalSymbol{symbol, "IndexedEventTxModel::Builder::withSymbol(symbol)"};

    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withStringSymbol, graalBuilder, graalSymbol.c_str());
}

void withSource(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const EventSourceWrapper &source) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withSource");
    const auto graalSource = source.toGraalUnique();

    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withSource, graalBuilder,
                       static_cast<dxfg_v4_indexed_event_source_t *>(graalSource.get()));
}

void withListener(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                  const JavaObjectHandle<IndexedEventTxModelListenerTag> &listener) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_IndexedEventTxModel_Builder_withListener`. The `listener` handle is "
                                       "invalid");
    }

    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withListener, graalBuilder,
                       static_cast<dxfg_v4_indexed_event_tx_model_listener_t *>(listener.get()));
}

void withEventsBatchLimit(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                          std::int32_t eventsBatchLimit) {
    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withEventsBatchLimit,
                       toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withEventsBatchLimit"), eventsBatchLimit);
}

void withAggregationPeriod(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                           std::int64_t aggregationPeriod) {
    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withAggregationPeriodMillis,
                       toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withAggregationPeriodMillis"),
                       aggregationPeriod);
}

void withExecutor(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, void *executor) {
    runGraalV4Function(dxfg_v4_IndexedEventTxModel_Builder_withExecutor,
                       toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_withExecutor"),
                       static_cast<dxfg_v4_executor_t *>(executor));
}

JavaObjectHandle<IndexedEventTxModelTag> build(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder) {
    return JavaObjectHandle<IndexedEventTxModelTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedEventTxModel_Builder_build, toGraal(builder, "dxfg_v4_IndexedEventTxModel_Builder_build"))};
}

} // namespace Builder

namespace Listener {

JavaObjectHandle<IndexedEventTxModelListenerTag> create(void *eventsReceived, void *userData) {
    if (!eventsReceived) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_IndexedEventTxModel_Listener_new`. The `eventsReceived` is nullptr");
    }

    // The listener functions are experimental, as the models are.
    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<IndexedEventTxModelListenerTag>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_IndexedEventTxModel_Listener_new,
        dxfcpp::bit_cast<dxfg_v4_IndexedEventTxModel_Listener_eventsReceived_f>(eventsReceived), userData)};
}

} // namespace Listener

} // namespace isolated::model::IsolatedIndexedEventTxModel

DXFCPP_END_NAMESPACE
