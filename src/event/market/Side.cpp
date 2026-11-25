// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Side.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const Side Side::UNDEFINED{0, "UNDEFINED"};
constinit const Side Side::BUY{1, "BUY"};
constinit const Side Side::SELL{2, "SELL"};

namespace {

constinit const std::array<std::reference_wrapper<const Side>, 3> ALL_ELEMENTS{
    std::cref(Side::UNDEFINED),
    std::cref(Side::BUY),
    std::cref(Side::SELL),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const Side>> Side::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE