// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/custom/NuamTimeInForceType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const NuamTimeInForceType NuamTimeInForceType::UNDEFINED{0, "UNDEFINED"};
constinit const NuamTimeInForceType NuamTimeInForceType::REST_OF_DAY{1, "REST_OF_DAY"};
constinit const NuamTimeInForceType NuamTimeInForceType::GTC{2, "GTC"};
constinit const NuamTimeInForceType NuamTimeInForceType::IMMEDIATE_OR_CANCEL{3, "IMMEDIATE_OR_CANCEL"};
constinit const NuamTimeInForceType NuamTimeInForceType::FILL_OR_KILL{4, "FILL_OR_KILL"};
constinit const NuamTimeInForceType NuamTimeInForceType::GOOD_TIL_SESSION{5, "GOOD_TIL_SESSION"};
constinit const NuamTimeInForceType NuamTimeInForceType::NUMBER_OF_DAYS{6, "NUMBER_OF_DAYS"};

namespace {

constinit const std::array<std::reference_wrapper<const NuamTimeInForceType>, 7> ALL_ELEMENTS{
    std::cref(NuamTimeInForceType::UNDEFINED),
    std::cref(NuamTimeInForceType::REST_OF_DAY),
    std::cref(NuamTimeInForceType::GTC),
    std::cref(NuamTimeInForceType::IMMEDIATE_OR_CANCEL),
    std::cref(NuamTimeInForceType::FILL_OR_KILL),
    std::cref(NuamTimeInForceType::GOOD_TIL_SESSION),
    std::cref(NuamTimeInForceType::NUMBER_OF_DAYS),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const NuamTimeInForceType>> NuamTimeInForceType::ALL{
    ALL_ELEMENTS};

DXFCPP_END_NAMESPACE
