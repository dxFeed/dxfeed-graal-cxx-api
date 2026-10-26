// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedEventTxModel.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedIndexedEventTxModel {

namespace {

dxfg_indexed_event_tx_model_t *toGraal(const JavaObjectHandle<IndexedEventTxModelTag> &model, const char *function) {
    if (!model) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `model` handle is invalid");
    }

    return static_cast<dxfg_indexed_event_tx_model_t *>(model.get());
}

dxfg_indexed_event_tx_model_builder_t *toGraal(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                               const char *function) {
    if (!builder) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `builder` handle is invalid");
    }

    return static_cast<dxfg_indexed_event_tx_model_builder_t *>(builder.get());
}

// A deleter of std::unique_ptr: a failure to free the symbol must not escape it.
void releaseSymbol(dxfg_symbol_t *symbol) noexcept {
    if (!symbol) {
        return;
    }

    runRelease("IndexedEventTxModel", [&] {
        runGraalFunctionAndThrowIfLessThanZero(dxfg_Symbol_release, symbol);
    });
}

// A deleter of std::unique_ptr: a failure to free the source must not escape it.
void releaseSource(dxfg_indexed_event_source_t *source) noexcept {
    if (!source) {
        return;
    }

    runRelease("IndexedEventTxModel", [&] {
        event::IsolatedIndexedEventSource::release(source);
    });
}

} // namespace

JavaObjectHandle<IndexedEventTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType) {
    dxfg_indexed_event_tx_model_builder_t *builder{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_newBuilder,
                                           static_cast<dxfg_event_clazz_t>(eventType.getId()), &builder);

    return JavaObjectHandle<IndexedEventTxModelBuilderTag>{builder};
}

IndexedEventSubscriptionSymbol
getIndexedEventSubscriptionSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    dxfg_symbol_t *symbol{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_getIndexedEventSubscriptionSymbol,
                                           toGraal(model, "dxfg_IndexedEventTxModel_getIndexedEventSubscriptionSymbol"),
                                           &symbol);

    const std::unique_ptr<dxfg_symbol_t, decltype(&releaseSymbol)> u{symbol, releaseSymbol};

    auto result = SymbolWrapper::fromGraal(symbol).asIndexedEventSubscriptionSymbol();

    if (!result) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_IndexedEventTxModel_getIndexedEventSubscriptionSymbol`. The result is "
                                       "not an IndexedEventSubscriptionSymbol");
    }

    return std::move(*result);
}

std::string getSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    const char *symbol{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_getSymbol,
                                           toGraal(model, "dxfg_IndexedEventTxModel_getSymbol"), &symbol);

    const auto u = internal::IsolatedString::toUnique(symbol);

    return symbol ? std::string(symbol) : std::string();
}

EventSourceWrapper getSource(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    dxfg_indexed_event_source_t *source{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_getSource,
                                           toGraal(model, "dxfg_IndexedEventTxModel_getSource"), &source);

    const std::unique_ptr<dxfg_indexed_event_source_t, decltype(&releaseSource)> u{source, releaseSource};

    return EventSourceWrapper::fromGraal(source);
}

JavaObjectHandle<SubscriptionController>
getSubscriptionController(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    dxfg_subscription_controller_t *controller{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_getSubscriptionController,
                                           toGraal(model, "dxfg_IndexedEventTxModel_getSubscriptionController"),
                                           &controller);

    return JavaObjectHandle<SubscriptionController>{controller};
}

bool isClosed(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    std::int32_t closed{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_isClosed,
                                           toGraal(model, "dxfg_IndexedEventTxModel_isClosed"), &closed);

    return closed == 1;
}

void close(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_close,
                                           toGraal(model, "dxfg_IndexedEventTxModel_close"));
}

namespace Builder {

void withFeed(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const JavaObjectHandle<DXFeed> &feed) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_IndexedEventTxModel_Builder_withFeed`. The `feed` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withFeed,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withFeed"),
                                           static_cast<dxfg_feed_t *>(feed.get()));
}

void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                const IndexedEventSubscriptionSymbol &symbol) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withSymbol,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withSymbol"),
                                           static_cast<dxfg_symbol_t *>(SymbolWrapper(symbol).toGraalUnique().get()));
}

void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const StringLike &symbol) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withSymbol2,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withSymbol2"),
                                           symbol.c_str());
}

void withSource(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const EventSourceWrapper &source) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withSource,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withSource"),
                                           static_cast<dxfg_indexed_event_source_t *>(source.toGraalUnique().get()));
}

void withListener(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                  const JavaObjectHandle<IndexedEventTxModelListenerTag> &listener) {
    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_IndexedEventTxModel_Builder_withListener`. "
                                       "The `listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withListener,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withListener"),
                                           static_cast<dxfg_indexed_event_tx_model_listener_t *>(listener.get()));
}

void withEventsBatchLimit(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                          std::int32_t eventsBatchLimit) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withEventsBatchLimit,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withEventsBatchLimit"),
                                           eventsBatchLimit);
}

void withAggregationPeriod(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                           std::int64_t aggregationPeriod) {
    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_IndexedEventTxModel_Builder_withAggregationPeriodMillis,
        toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withAggregationPeriodMillis"), aggregationPeriod);
}

void withExecutor(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, void *executor) {
    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_withExecutor,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_withExecutor"),
                                           static_cast<dxfg_executor_t *>(executor));
}

JavaObjectHandle<IndexedEventTxModelTag> build(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder) {
    dxfg_indexed_event_tx_model_t *model{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_IndexedEventTxModel_Builder_build,
                                           toGraal(builder, "dxfg_IndexedEventTxModel_Builder_build"), &model);

    return JavaObjectHandle<IndexedEventTxModelTag>{model};
}

} // namespace Builder

namespace Listener {

JavaObjectHandle<IndexedEventTxModelListenerTag> create(void *eventsReceived, void *userData) {
    if (!eventsReceived) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_IndexedEventTxModel_Listener_new`. The `eventsReceived` is nullptr");
    }

    dxfg_indexed_event_tx_model_listener_t *listener{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_IndexedEventTxModel_Listener_new,
        dxfcpp::bit_cast<dxfg_IndexedEventTxModel_Listener_eventsReceived_f>(eventsReceived), userData, &listener);

    return JavaObjectHandle<IndexedEventTxModelListenerTag>{listener};
}

} // namespace Listener

} // namespace isolated::model::IsolatedIndexedEventTxModel

DXFCPP_END_NAMESPACE
