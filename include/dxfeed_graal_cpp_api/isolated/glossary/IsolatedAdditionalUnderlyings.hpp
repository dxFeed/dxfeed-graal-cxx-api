// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../glossary/AdditionalUnderlyings.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedAdditionalUnderlyings {
// dxfg_AdditionalUnderlyings_EMPTY
JavaObjectHandle<AdditionalUnderlyings> EMPTY();

// int32_t dxfg_AdditionalUnderlyings_valueOf(graal_isolatethread_t *thread, const char *text, DXFG_OUT
// dxfg_additional_underlyings_t **additionalUnderlyings);
JavaObjectHandle<AdditionalUnderlyings> valueOf(const StringLikeWrapper &value);

// int32_t dxfg_AdditionalUnderlyings_valueOf2(graal_isolatethread_t *thread, const dxfg_string_to_double_map_entry_t *mapEntries, int32_t size, DXFG_OUT dxfg_additional_underlyings_t **additionalUnderlyings);
JavaObjectHandle<AdditionalUnderlyings> valueOf(const std::vector<std::pair<const char*, double>> & mapLikeEntries);

// int32_t dxfg_AdditionalUnderlyings_getSPC(graal_isolatethread_t *thread, const char *text, const char *symbol, DXFG_OUT double *spc);
double getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol);

// int32_t dxfg_AdditionalUnderlyings_getText(graal_isolatethread_t *thread, dxfg_additional_underlyings_t *additionalUnderlyings, DXFG_OUT char **text);
std::string getText(const JavaObjectHandle<AdditionalUnderlyings>& additionalUnderlyings);

} // namespace isolated::glossary::IsolatedAdditionalUnderlyings

DXFCPP_END_NAMESPACE