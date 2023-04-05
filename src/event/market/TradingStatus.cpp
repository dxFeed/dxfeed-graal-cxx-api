// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

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

} // namespace dxfcpp