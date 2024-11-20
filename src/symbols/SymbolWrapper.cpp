// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

std::ptrdiff_t SymbolWrapper::SymbolListUtils::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::calculateSize(initSize);
}

void *SymbolWrapper::SymbolListUtils::newGraalList(std::ptrdiff_t size) {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::newList(size);
}

bool SymbolWrapper::SymbolListUtils::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx,
                                                         void *element) noexcept {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::setElement(graalList, elementIdx, element);
}

bool SymbolWrapper::SymbolListUtils::freeGraalListElements(void *graalList, std::ptrdiff_t count) {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::freeElements(graalList, count);
}

void SymbolWrapper::SymbolListUtils::freeGraalList(void *graalList) {
    GraalListUtils<dxfg_symbol_list, SymbolWrapper>::freeList(graalList);
}

std::vector<SymbolWrapper> SymbolWrapper::SymbolListUtils::fromGraalList(void *graalList) {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::fromList(graalList);
}

template void *
SymbolWrapper::SymbolListUtils::toGraalList<dxfcpp::SymbolWrapper const *>(dxfcpp::SymbolWrapper const *,
                                                                           dxfcpp::SymbolWrapper const *);

void SymbolWrapper::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::SymbolListUtils::freeGraal(graalNative = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    switch (static_cast<dxfg_symbol_t *>(graalNative)->type) {
    case STRING:
        StringSymbol::freeGraal(graalNative);

        break;

    case CANDLE:
        CandleSymbol::freeGraal(graalNative);

        break;

    case WILDCARD:
        WildcardSymbol::freeGraal(graalNative);

        break;

    case INDEXED_EVENT_SUBSCRIPTION:
        IndexedEventSubscriptionSymbol::freeGraal(graalNative);

        break;

    case TIME_SERIES_SUBSCRIPTION:
        TimeSeriesSubscriptionSymbol::freeGraal(graalNative);

        break;
    }
}

SymbolWrapper SymbolWrapper::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::fromGraal(graalNative = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        throw InvalidArgumentException("Unable to create SymbolWrapper. The `graalNative` parameter is nullptr");
    }

    switch (static_cast<dxfg_symbol_t *>(graalNative)->type) {
    case STRING:
        return StringSymbol::fromGraal(graalNative);

    case CANDLE:
        return CandleSymbol::fromGraal(graalNative);

    case WILDCARD:
        return WildcardSymbol::fromGraal(graalNative);

    case INDEXED_EVENT_SUBSCRIPTION:
        return IndexedEventSubscriptionSymbol::fromGraal(graalNative);

    case TIME_SERIES_SUBSCRIPTION:
        return TimeSeriesSubscriptionSymbol::fromGraal(graalNative);

    default:
        throw RuntimeException(fmt::format("Unable to create SymbolWrapper. Unknown symbol type: {}",
                                             static_cast<int>(static_cast<dxfg_symbol_t *>(graalNative)->type)));
    }
}

DXFCPP_END_NAMESPACE
