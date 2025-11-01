// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TimeAndSaleType.hpp"

#include <type_traits>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const TimeAndSaleType TimeAndSaleType::NEW{0, "NEW"};
const TimeAndSaleType TimeAndSaleType::CORRECTION{1, "CORRECTION"};
const TimeAndSaleType TimeAndSaleType::CANCEL{2, "CANCEL"};

template <>
const std::unordered_map<TimeAndSaleType::CodeType, std::reference_wrapper<const TimeAndSaleType>>
    TimeAndSaleType::ParentType::ALL{
        {TimeAndSaleType::NEW.getCode(), std::cref(TimeAndSaleType::NEW)},
        {TimeAndSaleType::CORRECTION.getCode(), std::cref(TimeAndSaleType::CORRECTION)},
        {TimeAndSaleType::CANCEL.getCode(), std::cref(TimeAndSaleType::CANCEL)},
    };

DXFCPP_END_NAMESPACE