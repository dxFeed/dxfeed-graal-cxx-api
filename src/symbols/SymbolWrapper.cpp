// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

std::ptrdiff_t SymbolWrapper::SymbolListUtils::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (initSize < 0) {
        return 0;
    } else if (initSize > std::numeric_limits<SizeType>::max()) {
        return std::numeric_limits<SizeType>::max();
    }

    return initSize;
}

void *SymbolWrapper::SymbolListUtils::newGraalList(std::ptrdiff_t size) {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_symbol_t;

    auto *list = new ListType{static_cast<SizeType>(size), nullptr};

    if (size == 0) {
        return static_cast<void *>(list);
    }

    list->elements = new ElementType *[size] {
        nullptr
    };

    return list;
}

bool SymbolWrapper::SymbolListUtils::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx,
                                                         void *element) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_symbol_t;

    if (graalList == nullptr || elementIdx < 0 || elementIdx >= std::numeric_limits<SizeType>::max() ||
        element == nullptr) {
        return false;
    }

    static_cast<ListType *>(graalList)->elements[elementIdx] = static_cast<ElementType *>(element);

    return true;
}

bool SymbolWrapper::SymbolListUtils::freeGraalListElements(void *graalList, std::ptrdiff_t count) {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr || count < 0 || count >= std::numeric_limits<SizeType>::max()) {
        return false;
    }

    auto *list = static_cast<ListType *>(graalList);

    for (SizeType i = 0; i < count; i++) {
        SymbolWrapper::freeGraal(static_cast<void *>(list->elements[i]));
    }

    delete[] list->elements;
    delete list;

    return true;
}

void SymbolWrapper::SymbolListUtils::freeGraalList(void *graalList) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::SymbolListUtils::freeGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr) {
        return;
    }

    auto list = static_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                SymbolWrapper::freeGraal(static_cast<void *>(list->elements[elementIndex]));
            }
        }

        delete[] list->elements;
    }

    delete list;
}

std::vector<SymbolWrapper> SymbolWrapper::SymbolListUtils::fromGraalList(void *graalList) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::SymbolListUtils::fromGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<SymbolWrapper> result{};

    auto list = static_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                result.emplace_back(SymbolWrapper::fromGraal(static_cast<void *>(list->elements[elementIndex])));
            }
        }
    }

    return result;
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
        throw std::invalid_argument("Unable to create SymbolWrapper. The `graalNative` parameter is nullptr");
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
        throw std::runtime_error(fmt::format("Unable to create SymbolWrapper. Unknown symbol type: {}",
                                             static_cast<int>(static_cast<dxfg_symbol_t *>(graalNative)->type)));
    }

    return {};
}

DXFCPP_END_NAMESPACE
