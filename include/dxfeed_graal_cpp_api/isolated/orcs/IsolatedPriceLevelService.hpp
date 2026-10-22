// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../orcs/AuthOrderSource.hpp"
#include "../../orcs/PriceLevelService.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::orcs::IsolatedPriceLevelService {

// int32_t dxfg_PriceLevelService_new(graal_isolatethread_t *thread, const char *address,
// DXFG_OUT dxfg_price_level_service_t **service);
JavaObjectHandle<PriceLevelService> create(const StringLike &address);

// int32_t dxfg_PriceLevelService_getOrders(graal_isolatethread_t *thread, dxfg_price_level_service_t *service,
// dxfg_symbol_t *candleSymbol, dxfg_indexed_event_source_t *orderSource, int64_t from, int64_t to, const char *caller,
// DXFG_OUT dxfg_event_type_list **orders);
std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to, const StringLike &caller);

// int32_t dxfg_PriceLevelService_getOrders2(graal_isolatethread_t *thread, dxfg_price_level_service_t *service,
// dxfg_symbol_t *candleSymbol, dxfg_indexed_event_source_t *orderSource, int64_t from, int64_t to,
// DXFG_OUT dxfg_event_type_list **orders);
std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to);

// int32_t dxfg_PriceLevelService_getQuotes(graal_isolatethread_t *thread, dxfg_price_level_service_t *service,
// dxfg_symbol_t *candleSymbol, int64_t from, int64_t to, const char *caller, DXFG_OUT dxfg_event_type_list **quotes);
std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from, std::int64_t to,
                                                  const StringLike &caller);

// int32_t dxfg_PriceLevelService_getQuotes2(graal_isolatethread_t *thread, dxfg_price_level_service_t *service,
// dxfg_symbol_t *candleSymbol, int64_t from, int64_t to, DXFG_OUT dxfg_event_type_list **quotes);
std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from, std::int64_t to);

// int32_t dxfg_PriceLevelService_getAuthOrderSource(graal_isolatethread_t *thread, dxfg_price_level_service_t *service,
// DXFG_OUT dxfg_auth_order_source_t **authOrderSource);
JavaObjectHandle<AuthOrderSource> getAuthOrderSource(const JavaObjectHandle<PriceLevelService> &service);

// int32_t dxfg_PriceLevelService_close(graal_isolatethread_t *thread, dxfg_price_level_service_t *service);
void close(const JavaObjectHandle<PriceLevelService> &service);

} // namespace isolated::orcs::IsolatedPriceLevelService

namespace isolated::orcs::IsolatedAuthOrderSource {

// int32_t dxfg_AuthOrderSource_getByIds(graal_isolatethread_t *thread, dxfg_auth_order_source_t *authOrderSource,
// DXFG_OUT dxfg_symbols_by_order_source_id_map_entry_list_t **symbolsByOrderSourceIdMapEntryList);
// int32_t dxfg_symbols_by_order_source_id_map_entry_list_free(graal_isolatethread_t *thread,
// dxfg_symbols_by_order_source_id_map_entry_list_t *list);
std::unordered_map<std::int32_t, std::unordered_set<std::string>>
getByIds(const JavaObjectHandle<AuthOrderSource> &authOrderSource);

} // namespace isolated::orcs::IsolatedAuthOrderSource

namespace isolated::orcs::IsolatedPriceLevelChecker {

// int32_t dxfg_PriceLevelChecker_validate(graal_isolatethread_t *thread, dxfg_event_type_list *orders,
// int64_t timeGapBound, int32_t printQuotes, DXFG_OUT int32_t *isValid);
bool validate(const std::vector<std::shared_ptr<Order>> &orders, std::int64_t timeGapBound, bool printQuotes);

} // namespace isolated::orcs::IsolatedPriceLevelChecker

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
