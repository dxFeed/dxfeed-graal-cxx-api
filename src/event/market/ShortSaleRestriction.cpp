// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <utility>

namespace dxfcpp {

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

} // namespace dxfcpp