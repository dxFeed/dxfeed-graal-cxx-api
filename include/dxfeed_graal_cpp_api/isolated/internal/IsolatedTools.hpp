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

std::unordered_set<std::string> /* dxfg_string_list* */ parseSymbols(std::string_view symbolList);

void /* int32_t */ runTool(/* dxfg_string_list* */ const std::vector<std::string>& args);

}


DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()