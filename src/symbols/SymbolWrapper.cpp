// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>
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

std::unique_ptr<void, decltype(&SymbolWrapper::SymbolListUtils::freeGraalList)>
SymbolWrapper::SymbolListUtils::toGraalListUnique(std::initializer_list<SymbolWrapper> collection) {
    return {toGraalList(collection), freeGraalList};
}

std::vector<SymbolWrapper> SymbolWrapper::SymbolListUtils::fromGraalList(void *graalList) {
    return GraalListUtils<dxfg_symbol_list, SymbolWrapper>::fromList(graalList);
}

SymbolWrapper::SymbolWrapper(const SymbolWrapper &) noexcept = default;
SymbolWrapper::SymbolWrapper(SymbolWrapper &&) noexcept = default;
SymbolWrapper &SymbolWrapper::operator=(const SymbolWrapper &) noexcept = default;
SymbolWrapper &SymbolWrapper::operator=(SymbolWrapper &&) noexcept = default;
SymbolWrapper::SymbolWrapper() noexcept {
}
SymbolWrapper::~SymbolWrapper() noexcept {
}

SymbolWrapper::SymbolWrapper(const StringSymbol &stringSymbol) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper(stringSymbol = " + toStringAny(stringSymbol) + ")");
    }

    data_ = stringSymbol;
}

SymbolWrapper::SymbolWrapper(const WildcardSymbol &wildcardSymbol) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper(wildcardSymbol = " + toStringAny(wildcardSymbol) + ")");
    }

    data_ = wildcardSymbol;
}

SymbolWrapper::SymbolWrapper(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "SymbolWrapper(indexedEventSubscriptionSymbol = " + toStringAny(indexedEventSubscriptionSymbol) + ")");
    }

    data_ = indexedEventSubscriptionSymbol;
}

SymbolWrapper::SymbolWrapper(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper(timeSeriesSubscriptionSymbol = " + toStringAny(timeSeriesSubscriptionSymbol) +
                        ")");
    }

    data_ = timeSeriesSubscriptionSymbol;
}

SymbolWrapper::SymbolWrapper(const CandleSymbol &candleSymbol) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper(candleSymbol = " + toStringAny(candleSymbol) + ")");
    }

    data_ = candleSymbol;
}

template void *SymbolWrapper::SymbolListUtils::toGraalList<SymbolWrapper const *>(SymbolWrapper const *,
                                                                                  SymbolWrapper const *);

void SymbolWrapper::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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
        // ReSharper disable once CppDFAUnreachableCode
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

void *SymbolWrapper::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::toGraal()");
    }

    return std::visit(
        [](const auto &symbol) {
            return symbol.toGraal();
        },
        data_);
}

std::unique_ptr<void, decltype(&SymbolWrapper::freeGraal)> SymbolWrapper::toGraalUnique() const noexcept {
    return {toGraal(), SymbolWrapper::freeGraal};
}

std::string SymbolWrapper::toString() const {
    return "SymbolWrapper{" +
           std::visit(
               [](const auto &symbol) {
                   return toStringAny(symbol);
               },
               data_) +
           "}";
}

std::string SymbolWrapper::toStringUnderlying() const {
    return std::visit(
        [](const auto &symbol) {
            return toStringAny(symbol);
        },
        data_);
}

bool SymbolWrapper::isStringSymbol() const noexcept {
    return std::holds_alternative<StringSymbol>(data_);
}

std::string SymbolWrapper::asStringSymbol() const noexcept {
    return isStringSymbol() ? std::get<StringSymbol>(data_).getData() : String::EMPTY;
}

bool SymbolWrapper::isWildcardSymbol() const noexcept {
    return std::holds_alternative<WildcardSymbol>(data_);
}

std::optional<WildcardSymbol> SymbolWrapper::asWildcardSymbol() const noexcept {
    return isWildcardSymbol() ? std::make_optional(WildcardSymbol::ALL) : std::nullopt;
}

bool SymbolWrapper::isIndexedEventSubscriptionSymbol() const noexcept {
    return std::holds_alternative<IndexedEventSubscriptionSymbol>(data_);
}

std::optional<IndexedEventSubscriptionSymbol> SymbolWrapper::asIndexedEventSubscriptionSymbol() const noexcept {
    return isIndexedEventSubscriptionSymbol()
               ? std::make_optional<IndexedEventSubscriptionSymbol>(std::get<IndexedEventSubscriptionSymbol>(data_))
               : std::nullopt;
}

bool SymbolWrapper::isTimeSeriesSubscriptionSymbol() const noexcept {
    return std::holds_alternative<TimeSeriesSubscriptionSymbol>(data_);
}

std::optional<TimeSeriesSubscriptionSymbol> SymbolWrapper::asTimeSeriesSubscriptionSymbol() const noexcept {
    return isTimeSeriesSubscriptionSymbol()
               ? std::make_optional<TimeSeriesSubscriptionSymbol>(std::get<TimeSeriesSubscriptionSymbol>(data_))
               : std::nullopt;
}

bool SymbolWrapper::isCandleSymbol() const noexcept {
    return std::holds_alternative<CandleSymbol>(data_);
}

std::optional<CandleSymbol> SymbolWrapper::asCandleSymbol() const noexcept {
    return isCandleSymbol() ? std::make_optional<CandleSymbol>(std::get<CandleSymbol>(data_)) : std::nullopt;
}

const SymbolWrapper::DataType &SymbolWrapper::getData() const noexcept {
    return data_;
}

bool SymbolWrapper::operator==(const SymbolWrapper &symbolWrapper) const {
    return getData() == symbolWrapper.getData();
}

bool SymbolWrapper::operator<(const SymbolWrapper &symbolWrapper) const {
    return getData() < symbolWrapper.getData();
}

DXFCPP_END_NAMESPACE
