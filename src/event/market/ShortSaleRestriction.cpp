// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/ShortSaleRestriction.hpp"

#include <type_traits>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const ShortSaleRestriction ShortSaleRestriction::UNDEFINED{0, "UNDEFINED"};
const ShortSaleRestriction ShortSaleRestriction::ACTIVE{1, "ACTIVE"};
const ShortSaleRestriction ShortSaleRestriction::INACTIVE{2, "INACTIVE"};

template <>
const std::unordered_map<ShortSaleRestriction::CodeType, std::reference_wrapper<const ShortSaleRestriction>>
    ShortSaleRestriction::ParentType::ALL{
        {ShortSaleRestriction::UNDEFINED.getCode(), std::cref(ShortSaleRestriction::UNDEFINED)},
        {ShortSaleRestriction::ACTIVE.getCode(), std::cref(ShortSaleRestriction::ACTIVE)},
        {ShortSaleRestriction::INACTIVE.getCode(), std::cref(ShortSaleRestriction::INACTIVE)},
    };

DXFCPP_END_NAMESPACE