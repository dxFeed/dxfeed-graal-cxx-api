// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../candlewebservice/HistoryEndpoint.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::candlewebservice::IsolatedHistoryEndpoint {

namespace Builder {

// int32_t dxfg_HistoryEndpoint_Builder_withAddress(graal_isolatethread_t *thread, dxfg_history_endpoint_builder_t *builder, const char *address);
void withAddress(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLikeWrapper &address);

// int32_t dxfg_HistoryEndpoint_Builder_withUserName(graal_isolatethread_t *thread, dxfg_history_endpoint_builder_t *builder, const char *userName);
void withUserName(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLikeWrapper &userName);

// int32_t dxfg_HistoryEndpoint_Builder_withPassword(graal_isolatethread_t *thread, dxfg_history_endpoint_builder_t *builder, const char *password);
void withPassword(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLikeWrapper &password);

// int32_t dxfg_HistoryEndpoint_Builder_withAuthToken(graal_isolatethread_t *thread, dxfg_history_endpoint_builder_t *builder, const char *authToken);
void withAuthToken(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLikeWrapper &authToken);



} // namespace Builder

// int32_t dxfg_HistoryEndpoint_newBuilder(graal_isolatethread_t *thread, DXFG_OUT dxfg_history_endpoint_builder_t
// **builder);
JavaObjectHandle<HistoryEndpoint::Builder> newBuilder();

// int32_t dxfg_HistoryEndpoint_getTimeSeries(graal_isolatethread_t *thread, dxfg_history_endpoint_t *endpoint,
// dxfg_event_clazz_t eventClazz, dxfg_symbol_t *symbol, int64_t fromTime, int64_t toTime, DXFG_OUT dxfg_event_type_list
// **events);
std::vector<std::shared_ptr<EventType>> getTimeSeries(const JavaObjectHandle<HistoryEndpoint> &endpoint,
                                                      const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                                      std::int64_t fromTime, std::int64_t toTime);

} // namespace isolated::candlewebservice::IsolatedHistoryEndpoint

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()