// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/PriceType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const PriceType PriceType::REGULAR{0, "REGULAR"};
constinit const PriceType PriceType::INDICATIVE{1, "INDICATIVE"};
constinit const PriceType PriceType::PRELIMINARY{2, "PRELIMINARY"};
constinit const PriceType PriceType::FINAL{3, "FINAL"};

namespace {

constinit const std::array<std::reference_wrapper<const PriceType>, 4> ALL_ELEMENTS{
    std::cref(PriceType::REGULAR),
    std::cref(PriceType::INDICATIVE),
    std::cref(PriceType::PRELIMINARY),
    std::cref(PriceType::FINAL),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const PriceType>> PriceType::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE