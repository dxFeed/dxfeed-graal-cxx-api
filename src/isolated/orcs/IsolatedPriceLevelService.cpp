// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedPriceLevelService.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/EventSourceWrapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/CallbackBarrier.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::orcs::IsolatedPriceLevelService {

namespace {

void checkService(const JavaObjectHandle<PriceLevelService> &service, const char *function) {
    if (!service) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `service` handle is invalid");
    }
}

std::vector<std::shared_ptr<EventType>> toEvents(dxfg_v4_event_type_list *events) {
    const auto u = event::IsolatedEventTypeList::toUnique(events);

    return EventMapper::fromGraalList(u.get());
}

} // namespace

/// dxfg_v4_PriceLevelService_new
JavaObjectHandle<PriceLevelService> create(std::string_view address) {
    // The handle of the service is taken from the result, so the NULL result is an error.
    auto *service = runGraalV4FunctionWithNonNullResult(
        dxfg_v4_PriceLevelService_new, SdkString{address, "PriceLevelService::create(address)"}.c_str());

    return JavaObjectHandle<PriceLevelService>{&service->handler};
}

/// dxfg_v4_PriceLevelService_getOrdersWithCaller
std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to, std::string_view caller) {
    checkService(service, "dxfg_v4_PriceLevelService_getOrdersWithCaller");

    auto *orders = runGraalV4FunctionWithResult(
        dxfg_v4_PriceLevelService_getOrdersWithCaller, static_cast<dxfg_v4_price_level_service_t *>(service.get()),
        static_cast<dxfg_v4_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()),
        static_cast<dxfg_v4_indexed_event_source_t *>(EventSourceWrapper(orderSource).toGraalUnique().get()), from, to,
        SdkString{caller, "PriceLevelService::getOrders(caller)"}.c_str());

    return toEvents(orders);
}

/// dxfg_v4_PriceLevelService_getOrders
std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to) {
    checkService(service, "dxfg_v4_PriceLevelService_getOrders");

    auto *orders = runGraalV4FunctionWithResult(
        dxfg_v4_PriceLevelService_getOrders, static_cast<dxfg_v4_price_level_service_t *>(service.get()),
        static_cast<dxfg_v4_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()),
        static_cast<dxfg_v4_indexed_event_source_t *>(EventSourceWrapper(orderSource).toGraalUnique().get()), from,
        to);

    return toEvents(orders);
}

/// dxfg_v4_PriceLevelService_getQuotesWithCaller
std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from, std::int64_t to,
                                                  std::string_view caller) {
    checkService(service, "dxfg_v4_PriceLevelService_getQuotesWithCaller");

    auto *quotes = runGraalV4FunctionWithResult(
        dxfg_v4_PriceLevelService_getQuotesWithCaller, static_cast<dxfg_v4_price_level_service_t *>(service.get()),
        static_cast<dxfg_v4_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()), from, to,
        SdkString{caller, "PriceLevelService::getQuotes(caller)"}.c_str());

    return toEvents(quotes);
}

/// dxfg_v4_PriceLevelService_getQuotes
std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from,
                                                  std::int64_t to) {
    checkService(service, "dxfg_v4_PriceLevelService_getQuotes");

    auto *quotes = runGraalV4FunctionWithResult(
        dxfg_v4_PriceLevelService_getQuotes, static_cast<dxfg_v4_price_level_service_t *>(service.get()),
        static_cast<dxfg_v4_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()), from, to);

    return toEvents(quotes);
}

/// dxfg_v4_PriceLevelService_getAuthOrderSource
JavaObjectHandle<AuthOrderSource> getAuthOrderSource(const JavaObjectHandle<PriceLevelService> &service) {
    checkService(service, "dxfg_v4_PriceLevelService_getAuthOrderSource");

    // The handle of the source is taken from the result, so the NULL result is an error.
    auto *authOrderSource = runGraalV4FunctionWithNonNullResult(
        dxfg_v4_PriceLevelService_getAuthOrderSource, static_cast<dxfg_v4_price_level_service_t *>(service.get()));

    return JavaObjectHandle<AuthOrderSource>{&authOrderSource->handler};
}

/// dxfg_v4_PriceLevelService_close
void close(const JavaObjectHandle<PriceLevelService> &service) {
    checkService(service, "dxfg_v4_PriceLevelService_close");

    runGraalV4Function(dxfg_v4_PriceLevelService_close, static_cast<dxfg_v4_price_level_service_t *>(service.get()));
}

} // namespace isolated::orcs::IsolatedPriceLevelService

namespace isolated::orcs::IsolatedAuthOrderSource {

namespace {

// A deleter of std::unique_ptr: a failure to free the list must not escape it.
void freeList(dxfg_v4_symbols_by_order_source_id_map_entry_list *list) noexcept {
    if (!list) {
        return;
    }

    runRelease("PriceLevelService", [&] {
        runGraalV4Function(dxfg_v4_symbols_by_order_source_id_map_entry_list_release, list);
    });
}

} // namespace

/// dxfg_v4_AuthOrderSource_getByIds
std::unordered_map<std::int32_t, std::unordered_set<std::string>>
getByIds(const JavaObjectHandle<AuthOrderSource> &authOrderSource) {
    if (!authOrderSource) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthOrderSource_getByIds`. The `authOrderSource` handle is invalid");
    }

    auto *list = runGraalV4FunctionWithResult(dxfg_v4_AuthOrderSource_getByIds,
                                              static_cast<dxfg_v4_auth_order_source_t *>(authOrderSource.get()));

    // Frees the list, its entries and their symbol lists.
    const std::unique_ptr<dxfg_v4_symbols_by_order_source_id_map_entry_list, decltype(&freeList)> u{list, freeList};
    std::unordered_map<std::int32_t, std::unordered_set<std::string>> result{};

    if (!list || list->size <= 0 || !list->elements) {
        return result;
    }

    for (std::int32_t i = 0; i < list->size; i++) {
        const auto *entry = list->elements[i];

        if (!entry) {
            continue;
        }

        auto &symbols = result[entry->order_source_id];

        if (!entry->symbols || !entry->symbols->elements) {
            continue;
        }

        for (std::int32_t j = 0; j < entry->symbols->size; j++) {
            if (const auto *symbol = entry->symbols->elements[j]) {
                symbols.emplace(symbol);
            }
        }
    }

    return result;
}

} // namespace isolated::orcs::IsolatedAuthOrderSource

namespace isolated::orcs::IsolatedPriceLevelChecker {

/// dxfg_v4_PriceLevelChecker_validate
bool validate(const std::vector<std::shared_ptr<Order>> &orders, std::int64_t timeGapBound, bool printQuotes) {
    // The list stays with the caller: the SDK only reads it.
    const auto list = EventMapper::toGraalListUnique(orders.begin(), orders.end());

    return runGraalV4FunctionWithResult(dxfg_v4_PriceLevelChecker_validate,
                                        static_cast<dxfg_v4_event_type_list *>(list.get()), timeGapBound,
                                        printQuotes ? 1 : 0) != 0;
}

} // namespace isolated::orcs::IsolatedPriceLevelChecker

DXFCPP_END_NAMESPACE
