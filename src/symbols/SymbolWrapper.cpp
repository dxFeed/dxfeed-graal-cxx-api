// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void SymbolWrapper::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::freeGraal(graal = " + toStringAny(graal) + ")");
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
        Debugger::debug("SymbolWrapper::fromGraal(graal = " + toStringAny(graal) + ")");
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

    return {};
}

std::ptrdiff_t SymbolWrapper::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (initSize < 0) {
        return 0;
    } else if (initSize > std::numeric_limits<SizeType>::max()) {
        return std::numeric_limits<SizeType>::max();
    }

    return initSize;
}

void *SymbolWrapper::newGraalList(std::ptrdiff_t size) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_symbol_t;

    auto *list = new (std::nothrow) ListType{static_cast<SizeType>(size), nullptr};

    if (!list) {
        // TODO: error handling
        return nullptr;
    }

    if (size == 0) {
        return bit_cast<void *>(list);
    }

    list->elements = new (std::nothrow) ElementType *[size] { nullptr };

    if (!list->elements) {
        // TODO: error handling
        delete list;

        return nullptr;
    }

    return list;
}

bool SymbolWrapper::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_symbol_t;

    if (graalList == nullptr || elementIdx < 0 || elementIdx >= std::numeric_limits<SizeType>::max() ||
        element == nullptr) {
        return false;
    }

    bit_cast<ListType *>(graalList)->elements[elementIdx] = bit_cast<ElementType *>(element);

    return true;
}

bool SymbolWrapper::freeGraalListElements(void *graalList, std::ptrdiff_t count) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_symbol_t;

    if (graalList == nullptr || count < 0 || count >= std::numeric_limits<SizeType>::max()) {
        return false;
    }

    auto *list = bit_cast<ListType *>(graalList);

    for (SizeType i = 0; i < count; i++) {
        // TODO: error handling
        SymbolWrapper::freeGraal(bit_cast<void *>(list->elements[i]));
    }

    delete[] list->elements;
    delete list;

    return true;
}

void SymbolWrapper::freeGraalList(void *graalList) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::freeGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr) {
        return;
    }

    auto list = bit_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                freeGraal(bit_cast<void *>(list->elements[elementIndex]));
            }
        }

        delete[] list->elements;
    }

    delete list;
}

std::vector<SymbolWrapper> SymbolWrapper::fromGraalList(void *graalList) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("SymbolWrapper::fromGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<SymbolWrapper> result{};

    auto list = bit_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                result.emplace_back(fromGraal(bit_cast<void *>(list->elements[elementIndex])));
            }
        }
    }

    return result;
}

template void *SymbolWrapper::toGraalList<dxfcpp::SymbolWrapper const *>(dxfcpp::SymbolWrapper const *,
                                                                         dxfcpp::SymbolWrapper const *) noexcept;

} // namespace dxfcpp
