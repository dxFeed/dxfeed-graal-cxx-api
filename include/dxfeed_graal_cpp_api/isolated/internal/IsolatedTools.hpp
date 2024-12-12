// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>
#include <string_view>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal::IsolatedTools {

/**
 * Calls the Graal SDK function `dxfg_Tools_parseSymbols` in isolation.
 *
 * @param symbolList The symbol list
 * @return parsed symbols.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::unordered_set<std::string> /* dxfg_string_list* */ parseSymbols(std::string_view symbolList);

/**
 * Calls the Graal SDK function `dxfg_Tools_main` in isolation.
 *
 * @param args The tool's arguments
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ runTool(/* dxfg_string_list* */ const std::vector<std::string> &args);

} // namespace isolated::internal::IsolatedTools

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()