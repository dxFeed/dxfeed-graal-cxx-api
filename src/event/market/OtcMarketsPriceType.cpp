// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <functional>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const OtcMarketsPriceType OtcMarketsPriceType::UNPRICED{0, "UNPRICED"};
const OtcMarketsPriceType OtcMarketsPriceType::ACTUAL{1, "ACTUAL"};
const OtcMarketsPriceType OtcMarketsPriceType::WANTED{2, "WANTED"};

template <>
const std::unordered_map<OtcMarketsPriceType::CodeType, std::reference_wrapper<const OtcMarketsPriceType>>
    OtcMarketsPriceType::ParentType::ALL{
        {OtcMarketsPriceType::UNPRICED.getCode(), std::cref(OtcMarketsPriceType::UNPRICED)},
        {OtcMarketsPriceType::ACTUAL.getCode(), std::cref(OtcMarketsPriceType::ACTUAL)},
        {OtcMarketsPriceType::WANTED.getCode(), std::cref(OtcMarketsPriceType::WANTED)},
    };

DXFCPP_END_NAMESPACE