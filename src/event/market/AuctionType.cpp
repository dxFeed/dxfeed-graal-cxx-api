// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/AuctionType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const AuctionType AuctionType::UNDEFINED{0, "UNDEFINED"};
constinit const AuctionType AuctionType::OPENING{1, "OPENING"};
constinit const AuctionType AuctionType::CLOSING{2, "CLOSING"};
constinit const AuctionType AuctionType::REOPENING{3, "REOPENING"};
constinit const AuctionType AuctionType::OTHER{4, "OTHER"};

namespace {

constinit const std::array<std::reference_wrapper<const AuctionType>, 5> ALL_ELEMENTS{
    std::cref(AuctionType::UNDEFINED), std::cref(AuctionType::OPENING), std::cref(AuctionType::CLOSING),
    std::cref(AuctionType::REOPENING), std::cref(AuctionType::OTHER),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const AuctionType>> AuctionType::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE
