// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedTools.hpp"

#include "../IsolatedCommon.hpp"
#include "IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

void gc() {
    isolated::runGraalV4Function(dxfg_v4_gc);
}

namespace isolated::internal::IsolatedTools {
std::unordered_set<std::string> /* dxfg_v4_string_list* */
parseSymbols(std::string_view symbolList) {
    std::unordered_set<std::string> result{};

    const auto graalStringList =
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Tools_parseSymbols,
                                            SdkString{symbolList, "CmdArgsUtils::parseSymbols(symbols)"}.c_str());

    for (auto i = 0; i < graalStringList->size; i++) {
        result.emplace(dxfcpp::toString(graalStringList->elements[i]));
    }

    IsolatedStringList::release(graalStringList);

    return result;
}

std::vector<std::string> /* dxfg_v4_string_list* */ parseSymbolsAndSaveOrder(std::string_view symbolList) {
    std::vector<std::string> result{};

    const auto graalStringList =
        runGraalV4FunctionWithNonNullResult(dxfg_v4_Tools_parseSymbols,
                                            SdkString{symbolList, "CmdArgsUtils::parseSymbolsAndSaveOrder(symbols)"}
                                                .c_str());

    if (graalStringList->size > 0) {
        result.reserve(static_cast<std::size_t>(graalStringList->size));
    }

    for (auto i = 0; i < graalStringList->size; i++) {
        result.emplace_back(dxfcpp::toString(graalStringList->elements[i]));
    }

    IsolatedStringList::release(graalStringList);

    return result;
}

using NativeStringList = NativeStringListWrapper<dxfg_v4_string_list>;

void /* int32_t */ runTool(/* dxfg_v4_string_list* */ const std::vector<std::string> &args) {
    NativeStringList l{args, "IsolatedTools::runTool(args)"};

    runGraalV4Function(dxfg_v4_Tools_main, l.list);
}
} // namespace isolated::internal::IsolatedTools

DXFCPP_END_NAMESPACE
