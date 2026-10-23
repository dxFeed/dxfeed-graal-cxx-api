// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/ImbalanceSide.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const ImbalanceSide ImbalanceSide::UNDEFINED{0, "UNDEFINED"};
constinit const ImbalanceSide ImbalanceSide::BUY{1, "BUY"};
constinit const ImbalanceSide ImbalanceSide::SELL{2, "SELL"};
constinit const ImbalanceSide ImbalanceSide::NO_IMBALANCE{3, "NO_IMBALANCE"};
constinit const ImbalanceSide ImbalanceSide::INSUFFICIENT_ORDERS{4, "INSUFFICIENT_ORDERS"};
constinit const ImbalanceSide ImbalanceSide::PAUSED{5, "PAUSED"};

namespace {

constinit const std::array<std::reference_wrapper<const ImbalanceSide>, 6> ALL_ELEMENTS{
    std::cref(ImbalanceSide::UNDEFINED),
    std::cref(ImbalanceSide::BUY),
    std::cref(ImbalanceSide::SELL),
    std::cref(ImbalanceSide::NO_IMBALANCE),
    std::cref(ImbalanceSide::INSUFFICIENT_ORDERS),
    std::cref(ImbalanceSide::PAUSED),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const ImbalanceSide>> ImbalanceSide::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE
