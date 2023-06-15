// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void SymbolWrapper::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "SymbolWrapper::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

    switch (dxfcpp::bit_cast<dxfg_symbol_t *>(graal)->type) {
    case STRING:
        StringSymbol::freeGraal(graal);

        break;

    case CANDLE:
        // TODO: implement

        break;

    case WILDCARD:
        WildcardSymbol::freeGraal(graal);

        break;

    case INDEXED_EVENT_SUBSCRIPTION:
        IndexedEventSubscriptionSymbol::freeGraal(graal);

        break;

    case TIME_SERIES_SUBSCRIPTION:
        TimeSeriesSubscriptionSymbol::freeGraal(graal);

        break;
    }
}

SymbolWrapper SymbolWrapper::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "SymbolWrapper::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

    switch (dxfcpp::bit_cast<dxfg_symbol_t *>(graal)->type) {
    case STRING:
        return StringSymbol::fromGraal(graal);

    case CANDLE:
        // TODO: implement

        return {};

    case WILDCARD:
        return WildcardSymbol::fromGraal(graal);

    case INDEXED_EVENT_SUBSCRIPTION:
        return IndexedEventSubscriptionSymbol::fromGraal(graal);

    case TIME_SERIES_SUBSCRIPTION:
        return TimeSeriesSubscriptionSymbol::fromGraal(graal);
    }
}

} // namespace dxfcpp