// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

template <typename SymbolIt> void *SymbolMapper::toGraalList(SymbolIt begin, SymbolIt end) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);
    using ElementType = RemoveAllPointers<decltype(ListType::elements)>;

    auto size = std::distance(begin, end);

    if (size < 0) {
        size = 0;
    }

    if (size > std::numeric_limits<SizeType>::max()) {
        size = std::numeric_limits<SizeType>::max();
    }

    // Zero size is needed, for example, to clear the list of symbols.
    ListType *list = new (std::nothrow) ListType{static_cast<SizeType>(size), nullptr};

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

    SizeType elementIdx = 0;
    bool needToFree = false;

    for (auto it = begin; it != end && elementIdx < size; it++, elementIdx++) {
        if constexpr (requires { it->toGraal(); }) {
            list->elements[elementIdx] = bit_cast<ElementType *>(it->toGraal());
        } else if constexpr (std::is_convertible_v<decltype(*it), SymbolWrapper> ||
                             ConvertibleToSymbolWrapper<decltype(*it)>) {
            list->elements[elementIdx] = bit_cast<ElementType *>(SymbolWrapper(*it).toGraal());
        }

        if (!list->elements[elementIdx]) {
            needToFree = true;

            break;
        }
    }

    if (needToFree) {
        for (SizeType i = 0; i < elementIdx; i++) {
            // TODO: error handling
            delete list->elements[i];
        }

        delete[] list->elements;

        return nullptr;
    }

    return bit_cast<void *>(list);
}

void SymbolMapper::freeGraalList(void *graalList) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr) {
        return;
    }

    auto list = bit_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                SymbolWrapper::freeGraal(bit_cast<void *>(list->elements[elementIndex]));
            }
        }

        delete[] list->elements;
    }

    delete list;
}

std::vector<SymbolWrapper> SymbolMapper::fromGraal(void *graalList) noexcept {
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
                result.emplace_back(SymbolWrapper::fromGraal(bit_cast<void *>(list->elements[elementIndex])));
            }
        }
    }

    return result;
}

} // namespace dxfcpp