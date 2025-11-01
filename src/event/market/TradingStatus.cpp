// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TradingStatus.hpp"

#include <type_traits>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const TradingStatus TradingStatus::UNDEFINED{0, "UNDEFINED"};
const TradingStatus TradingStatus::HALTED{1, "HALTED"};
const TradingStatus TradingStatus::ACTIVE{2, "ACTIVE"};

template <>
const std::unordered_map<TradingStatus::CodeType, std::reference_wrapper<const TradingStatus>>
    TradingStatus::ParentType::ALL{
        {TradingStatus::UNDEFINED.getCode(), std::cref(TradingStatus::UNDEFINED)},
        {TradingStatus::HALTED.getCode(), std::cref(TradingStatus::HALTED)},
        {TradingStatus::ACTIVE.getCode(), std::cref(TradingStatus::ACTIVE)},
    };

DXFCPP_END_NAMESPACE