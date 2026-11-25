// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OtcMarketsPriceType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const OtcMarketsPriceType OtcMarketsPriceType::UNPRICED{0, "UNPRICED"};
constinit const OtcMarketsPriceType OtcMarketsPriceType::ACTUAL{1, "ACTUAL"};
constinit const OtcMarketsPriceType OtcMarketsPriceType::WANTED{2, "WANTED"};

namespace {

constinit const std::array<std::reference_wrapper<const OtcMarketsPriceType>, 3> ALL_ELEMENTS{
    std::cref(OtcMarketsPriceType::UNPRICED),
    std::cref(OtcMarketsPriceType::ACTUAL),
    std::cref(OtcMarketsPriceType::WANTED),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const OtcMarketsPriceType>> OtcMarketsPriceType::ALL{
    ALL_ELEMENTS};

DXFCPP_END_NAMESPACE