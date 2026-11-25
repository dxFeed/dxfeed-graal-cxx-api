// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/ShortSaleRestriction.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const ShortSaleRestriction ShortSaleRestriction::UNDEFINED{0, "UNDEFINED"};
constinit const ShortSaleRestriction ShortSaleRestriction::ACTIVE{1, "ACTIVE"};
constinit const ShortSaleRestriction ShortSaleRestriction::INACTIVE{2, "INACTIVE"};

namespace {

constinit const std::array<std::reference_wrapper<const ShortSaleRestriction>, 3> ALL_ELEMENTS{
    std::cref(ShortSaleRestriction::UNDEFINED),
    std::cref(ShortSaleRestriction::ACTIVE),
    std::cref(ShortSaleRestriction::INACTIVE),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const ShortSaleRestriction>> ShortSaleRestriction::ALL{
    ALL_ELEMENTS};

DXFCPP_END_NAMESPACE