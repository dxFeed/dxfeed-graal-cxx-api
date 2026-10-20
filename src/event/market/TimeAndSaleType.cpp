// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TimeAndSaleType.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const TimeAndSaleType TimeAndSaleType::NEW{0, "NEW"};
constinit const TimeAndSaleType TimeAndSaleType::CORRECTION{1, "CORRECTION"};
constinit const TimeAndSaleType TimeAndSaleType::CANCEL{2, "CANCEL"};

namespace {

constinit const std::array<std::reference_wrapper<const TimeAndSaleType>, 3> ALL_ELEMENTS{
    std::cref(TimeAndSaleType::NEW),
    std::cref(TimeAndSaleType::CORRECTION),
    std::cref(TimeAndSaleType::CANCEL),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const TimeAndSaleType>> TimeAndSaleType::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE