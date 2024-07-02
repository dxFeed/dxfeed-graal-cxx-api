// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedTools.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal::IsolatedTools {
std::unordered_set<std::string> /* dxfg_string_list* */
parseSymbols(std::string_view symbolList) {
    std::unordered_set<std::string> result{};

    auto graalStringList = runGraalFunctionAndThrowIfNullptr(dxfg_Tools_parseSymbols, symbolList.data());

    for (auto i = 0; i < graalStringList->size; i++) {
        result.emplace(dxfcpp::toString(graalStringList->elements[i]));
    }

    IsolatedStringList::release(graalStringList);

    return result;
}

using NativeStringList = NativeStringListWrapper<dxfg_string_list>;

void /* int32_t */ runTool(/* dxfg_string_list* */ const std::vector<std::string> &args) {
    NativeStringList l{args};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_Tools_main, l.list);
}
} // namespace isolated::internal::IsolatedTools

DXFCPP_END_NAMESPACE