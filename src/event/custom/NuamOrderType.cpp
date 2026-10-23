// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/custom/NuamOrderType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const NuamOrderType NuamOrderType::UNDEFINED{0, "UNDEFINED"};
constinit const NuamOrderType NuamOrderType::LIMIT{1, "LIMIT"};
constinit const NuamOrderType NuamOrderType::MARKET{2, "MARKET"};
constinit const NuamOrderType NuamOrderType::MARKET_TO_LIMIT{3, "MARKET_TO_LIMIT"};
constinit const NuamOrderType NuamOrderType::BEST_ORDER{4, "BEST_ORDER"};
constinit const NuamOrderType NuamOrderType::IMBALANCE{5, "IMBALANCE"};

namespace {

constinit const std::array<std::reference_wrapper<const NuamOrderType>, 6> ALL_ELEMENTS{
    std::cref(NuamOrderType::UNDEFINED),  std::cref(NuamOrderType::LIMIT),
    std::cref(NuamOrderType::MARKET),     std::cref(NuamOrderType::MARKET_TO_LIMIT),
    std::cref(NuamOrderType::BEST_ORDER), std::cref(NuamOrderType::IMBALANCE),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const NuamOrderType>> NuamOrderType::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE
