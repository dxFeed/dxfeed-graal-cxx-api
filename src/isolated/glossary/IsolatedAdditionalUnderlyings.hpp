// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

#include "../../../include/dxfeed_graal_cpp_api/glossary/AdditionalUnderlyings.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedAdditionalUnderlyings {
/// dxfg_v4_AdditionalUnderlyings_EMPTY
/* dxfg_v4_additional_underlyings_t* */ JavaObjectHandle<AdditionalUnderlyings> EMPTY();

/// dxfg_v4_AdditionalUnderlyings_valueOf
/* dxfg_v4_additional_underlyings_t* */ JavaObjectHandle<AdditionalUnderlyings> valueOf(std::string_view value);

/// dxfg_v4_AdditionalUnderlyings_valueOfMap
/* dxfg_v4_additional_underlyings_t* */ JavaObjectHandle<AdditionalUnderlyings>
valueOf(const std::vector<std::pair<const char *, double>> &mapLikeEntries);

/// dxfg_v4_AdditionalUnderlyings_getSPCFromText
double getSPC(std::string_view text, std::string_view symbol);

/// dxfg_v4_AdditionalUnderlyings_getText
std::string
getText(/* dxfg_v4_additional_underlyings_t* */ const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings);

/// dxfg_v4_AdditionalUnderlyings_getMap
std::unordered_map<std::string, double>
getMap(/* dxfg_v4_additional_underlyings_t* */ const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings);

/// dxfg_v4_AdditionalUnderlyings_getSPC
double
getSPC(/* dxfg_v4_additional_underlyings_t* */ const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings,
       std::string_view symbol);

} // namespace isolated::glossary::IsolatedAdditionalUnderlyings

DXFCPP_END_NAMESPACE
