// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/orcs/IsolatedPriceLevelService.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/EventSourceWrapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp"
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

std::vector<std::shared_ptr<EventType>> toEvents(dxfg_event_type_list *events) {
    const auto u = event::IsolatedEventTypeList::toUnique(events);

    return EventMapper::fromGraalList(u.get());
}

} // namespace

JavaObjectHandle<PriceLevelService> create(const StringLike &address) {
    dxfg_price_level_service_t *service{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_PriceLevelService_new, address.c_str(), &service);

    return JavaObjectHandle<PriceLevelService>{&service->handler};
}

std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to, const StringLike &caller) {
    checkService(service, "dxfg_PriceLevelService_getOrders");

    dxfg_event_type_list *orders{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_PriceLevelService_getOrders, static_cast<dxfg_price_level_service_t *>(service.get()),
        static_cast<dxfg_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()),
        static_cast<dxfg_indexed_event_source_t *>(EventSourceWrapper(orderSource).toGraalUnique().get()), from, to,
        caller.c_str(), &orders);

    return toEvents(orders);
}

std::vector<std::shared_ptr<EventType>> getOrders(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to) {
    checkService(service, "dxfg_PriceLevelService_getOrders2");

    dxfg_event_type_list *orders{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_PriceLevelService_getOrders2, static_cast<dxfg_price_level_service_t *>(service.get()),
        static_cast<dxfg_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()),
        static_cast<dxfg_indexed_event_source_t *>(EventSourceWrapper(orderSource).toGraalUnique().get()), from, to,
        &orders);

    return toEvents(orders);
}

std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from, std::int64_t to,
                                                  const StringLike &caller) {
    checkService(service, "dxfg_PriceLevelService_getQuotes");

    dxfg_event_type_list *quotes{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_PriceLevelService_getQuotes, static_cast<dxfg_price_level_service_t *>(service.get()),
        static_cast<dxfg_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()), from, to, caller.c_str(),
        &quotes);

    return toEvents(quotes);
}

std::vector<std::shared_ptr<EventType>> getQuotes(const JavaObjectHandle<PriceLevelService> &service,
                                                  const CandleSymbol &candleSymbol, std::int64_t from,
                                                  std::int64_t to) {
    checkService(service, "dxfg_PriceLevelService_getQuotes2");

    dxfg_event_type_list *quotes{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_PriceLevelService_getQuotes2, static_cast<dxfg_price_level_service_t *>(service.get()),
        static_cast<dxfg_symbol_t *>(SymbolWrapper(candleSymbol).toGraalUnique().get()), from, to, &quotes);

    return toEvents(quotes);
}

JavaObjectHandle<AuthOrderSource> getAuthOrderSource(const JavaObjectHandle<PriceLevelService> &service) {
    checkService(service, "dxfg_PriceLevelService_getAuthOrderSource");

    dxfg_auth_order_source_t *authOrderSource{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_PriceLevelService_getAuthOrderSource,
                                           static_cast<dxfg_price_level_service_t *>(service.get()), &authOrderSource);

    return JavaObjectHandle<AuthOrderSource>{&authOrderSource->handler};
}

void close(const JavaObjectHandle<PriceLevelService> &service) {
    checkService(service, "dxfg_PriceLevelService_close");

    runGraalFunctionAndThrowIfLessThanZero(dxfg_PriceLevelService_close,
                                           static_cast<dxfg_price_level_service_t *>(service.get()));
}

} // namespace isolated::orcs::IsolatedPriceLevelService

namespace isolated::orcs::IsolatedAuthOrderSource {

namespace {

// A deleter of std::unique_ptr: a failure to free the list must not escape it.
void freeList(dxfg_symbols_by_order_source_id_map_entry_list_t *list) noexcept {
    if (!list) {
        return;
    }

    runRelease("PriceLevelService", [&] {
        runGraalFunctionAndThrowIfLessThanZero(dxfg_symbols_by_order_source_id_map_entry_list_free, list);
    });
}

} // namespace

std::unordered_map<std::int32_t, std::unordered_set<std::string>>
getByIds(const JavaObjectHandle<AuthOrderSource> &authOrderSource) {
    if (!authOrderSource) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthOrderSource_getByIds`. The `authOrderSource` handle is invalid");
    }

    dxfg_symbols_by_order_source_id_map_entry_list_t *list{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_AuthOrderSource_getByIds,
                                           static_cast<dxfg_auth_order_source_t *>(authOrderSource.get()), &list);

    const std::unique_ptr<dxfg_symbols_by_order_source_id_map_entry_list_t, decltype(&freeList)> u{list, freeList};
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

bool validate(const std::vector<std::shared_ptr<Order>> &orders, std::int64_t timeGapBound, bool printQuotes) {
    const auto list = EventMapper::toGraalListUnique(orders.begin(), orders.end());
    std::int32_t isValid{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_PriceLevelChecker_validate,
                                           static_cast<dxfg_event_type_list *>(list.get()), timeGapBound,
                                           printQuotes ? 1 : 0, &isValid);

    return isValid != 0;
}

} // namespace isolated::orcs::IsolatedPriceLevelChecker

DXFCPP_END_NAMESPACE
