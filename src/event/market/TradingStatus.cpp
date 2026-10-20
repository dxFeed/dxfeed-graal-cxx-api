// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TradingStatus.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const TradingStatus TradingStatus::UNDEFINED{0, "UNDEFINED"};
constinit const TradingStatus TradingStatus::HALTED{1, "HALTED"};
constinit const TradingStatus TradingStatus::ACTIVE{2, "ACTIVE"};

namespace {

constinit const std::array<std::reference_wrapper<const TradingStatus>, 3> ALL_ELEMENTS{
    std::cref(TradingStatus::UNDEFINED),
    std::cref(TradingStatus::HALTED),
    std::cref(TradingStatus::ACTIVE),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const TradingStatus>> TradingStatus::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE