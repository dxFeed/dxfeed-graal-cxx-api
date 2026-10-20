// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfileType.hpp"

#include <array>
#include <functional>
#include <span>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

constinit const InstrumentProfileType InstrumentProfileType::CURRENCY{0, "CURRENCY"};
constinit const InstrumentProfileType InstrumentProfileType::FOREX{1, "FOREX"};
constinit const InstrumentProfileType InstrumentProfileType::BOND{2, "BOND"};
constinit const InstrumentProfileType InstrumentProfileType::INDEX{3, "INDEX"};
constinit const InstrumentProfileType InstrumentProfileType::STOCK{4, "STOCK"};
constinit const InstrumentProfileType InstrumentProfileType::ETF{5, "ETF"};
constinit const InstrumentProfileType InstrumentProfileType::MUTUAL_FUND{6, "MUTUAL_FUND"};
constinit const InstrumentProfileType InstrumentProfileType::MONEY_MARKET_FUND{7, "MONEY_MARKET_FUND"};
constinit const InstrumentProfileType InstrumentProfileType::PRODUCT{8, "PRODUCT"};
constinit const InstrumentProfileType InstrumentProfileType::FUTURE{9, "FUTURE"};
constinit const InstrumentProfileType InstrumentProfileType::OPTION{10, "OPTION"};
constinit const InstrumentProfileType InstrumentProfileType::WARRANT{11, "WARRANT"};
constinit const InstrumentProfileType InstrumentProfileType::CFD{12, "CFD"};
constinit const InstrumentProfileType InstrumentProfileType::SPREAD{13, "SPREAD"};
constinit const InstrumentProfileType InstrumentProfileType::OTHER{14, "OTHER"};
constinit const InstrumentProfileType InstrumentProfileType::REMOVED{15, "REMOVED"};

namespace {

constinit const std::array<std::reference_wrapper<const InstrumentProfileType>, 16> ALL_ELEMENTS{
    std::cref(InstrumentProfileType::CURRENCY),    std::cref(InstrumentProfileType::FOREX),
    std::cref(InstrumentProfileType::BOND),        std::cref(InstrumentProfileType::INDEX),
    std::cref(InstrumentProfileType::STOCK),       std::cref(InstrumentProfileType::ETF),
    std::cref(InstrumentProfileType::MUTUAL_FUND), std::cref(InstrumentProfileType::MONEY_MARKET_FUND),
    std::cref(InstrumentProfileType::PRODUCT),     std::cref(InstrumentProfileType::FUTURE),
    std::cref(InstrumentProfileType::OPTION),      std::cref(InstrumentProfileType::WARRANT),
    std::cref(InstrumentProfileType::CFD),         std::cref(InstrumentProfileType::SPREAD),
    std::cref(InstrumentProfileType::OTHER),       std::cref(InstrumentProfileType::REMOVED),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const InstrumentProfileType>> InstrumentProfileType::ALL{
    ALL_ELEMENTS};

DXFCPP_END_NAMESPACE