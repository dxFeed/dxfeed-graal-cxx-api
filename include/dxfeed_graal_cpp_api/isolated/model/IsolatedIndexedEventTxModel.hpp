// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXFeed.hpp"
#include "../../model/IndexedEventTxModel.hpp"

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::model::IsolatedIndexedEventTxModel {

// int32_t dxfg_IndexedEventTxModel_newBuilder(graal_isolatethread_t *thread, dxfg_event_clazz_t eventType,
// DXFG_OUT dxfg_indexed_event_tx_model_builder_t **builder);
JavaObjectHandle<IndexedEventTxModelBuilderTag> newBuilder(const EventTypeEnum &eventType);

// int32_t dxfg_IndexedEventTxModel_getIndexedEventSubscriptionSymbol(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_t *model, DXFG_OUT dxfg_symbol_t **symbol);
IndexedEventSubscriptionSymbol getIndexedEventSubscriptionSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model);

// int32_t dxfg_IndexedEventTxModel_getSymbol(graal_isolatethread_t *thread, dxfg_indexed_event_tx_model_t *model,
// DXFG_OUT const char **symbol);
std::string getSymbol(const JavaObjectHandle<IndexedEventTxModelTag> &model);

// int32_t dxfg_IndexedEventTxModel_getSource(graal_isolatethread_t *thread, dxfg_indexed_event_tx_model_t *model,
// DXFG_OUT dxfg_indexed_event_source_t **source);
EventSourceWrapper getSource(const JavaObjectHandle<IndexedEventTxModelTag> &model);

// int32_t dxfg_IndexedEventTxModel_getSubscriptionController(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_t *model, DXFG_OUT dxfg_subscription_controller_t **subscriptionController);
JavaObjectHandle<SubscriptionController>
getSubscriptionController(const JavaObjectHandle<IndexedEventTxModelTag> &model);

// int32_t dxfg_IndexedEventTxModel_isClosed(graal_isolatethread_t *thread, dxfg_indexed_event_tx_model_t *model,
// DXFG_OUT int32_t *isClosed);
bool isClosed(const JavaObjectHandle<IndexedEventTxModelTag> &model);

// int32_t dxfg_IndexedEventTxModel_close(graal_isolatethread_t *thread, dxfg_indexed_event_tx_model_t *model);
void close(const JavaObjectHandle<IndexedEventTxModelTag> &model);

namespace Builder {

// int32_t dxfg_IndexedEventTxModel_Builder_withFeed(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, dxfg_feed_t *feed);
void withFeed(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const JavaObjectHandle<DXFeed> &feed);

// int32_t dxfg_IndexedEventTxModel_Builder_withSymbol(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, dxfg_symbol_t *symbol);
void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                const IndexedEventSubscriptionSymbol &symbol);

// int32_t dxfg_IndexedEventTxModel_Builder_withSymbol2(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, const char *symbol);
void withSymbol(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const StringLike &symbol);

// int32_t dxfg_IndexedEventTxModel_Builder_withSource(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, dxfg_indexed_event_source_t *source);
void withSource(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, const EventSourceWrapper &source);

// int32_t dxfg_IndexedEventTxModel_Builder_withListener(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, dxfg_indexed_event_tx_model_listener_t *listener);
void withListener(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                  const JavaObjectHandle<IndexedEventTxModelListenerTag> &listener);

// int32_t dxfg_IndexedEventTxModel_Builder_withEventsBatchLimit(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, int32_t eventsBatchLimit);
void withEventsBatchLimit(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                          std::int32_t eventsBatchLimit);

// int32_t dxfg_IndexedEventTxModel_Builder_withAggregationPeriodMillis(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, int64_t aggregationPeriod);
void withAggregationPeriod(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                           std::int64_t aggregationPeriod);

// int32_t dxfg_IndexedEventTxModel_Builder_withExecutor(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, dxfg_executor_t *executor);
void withExecutor(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                  /* dxfg_executor_t* */ void *executor);

// int32_t dxfg_IndexedEventTxModel_Builder_build(graal_isolatethread_t *thread,
// dxfg_indexed_event_tx_model_builder_t *builder, DXFG_OUT dxfg_indexed_event_tx_model_t **model);
JavaObjectHandle<IndexedEventTxModelTag> build(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder);

} // namespace Builder

namespace Listener {

// int32_t dxfg_IndexedEventTxModel_Listener_new(graal_isolatethread_t *thread,
// dxfg_IndexedEventTxModel_Listener_eventsReceived_f eventsReceived, void *userData,
// DXFG_OUT dxfg_indexed_event_tx_model_listener_t **listener);
JavaObjectHandle<IndexedEventTxModelListenerTag>
create(/* dxfg_IndexedEventTxModel_Listener_eventsReceived_f */ void *eventsReceived, void *userData);

} // namespace Listener

} // namespace isolated::model::IsolatedIndexedEventTxModel

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
