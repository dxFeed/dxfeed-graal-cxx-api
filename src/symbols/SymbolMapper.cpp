// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

template <typename SymbolIt> void *SymbolMapper::toNativeList(SymbolIt begin, SymbolIt end) noexcept {
    using ListType = dxfg_symbol_list;
    using SizeType = decltype(dxfg_symbol_list::size);
    using ElelemntType = RemoveAllPointers<decltype(dxfg_symbol_list::elements)>;

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
        return list;
    }

    return list;
}

} // namespace dxfcpp