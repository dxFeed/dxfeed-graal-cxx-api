// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

/**
 * Defines standard types of InstrumentProfile. Note that other (unknown) types
 * can be used without listing in this class - use it for convenience only.
 * Please see <b>Instrument Profile Format</b> documentation for complete description.
 */
struct InstrumentProfileType : Enum<InstrumentProfileType, std::uint32_t> {
    using Enum::Enum;

    static const DXFCPP_EXPORT InstrumentProfileType CURRENCY;
    static const DXFCPP_EXPORT InstrumentProfileType FOREX;
    static const DXFCPP_EXPORT InstrumentProfileType BOND;
    static const DXFCPP_EXPORT InstrumentProfileType INDEX;
    static const DXFCPP_EXPORT InstrumentProfileType STOCK;
    static const DXFCPP_EXPORT InstrumentProfileType ETF;
    static const DXFCPP_EXPORT InstrumentProfileType MUTUAL_FUND;
    static const DXFCPP_EXPORT InstrumentProfileType MONEY_MARKET_FUND;
    static const DXFCPP_EXPORT InstrumentProfileType PRODUCT;
    static const DXFCPP_EXPORT InstrumentProfileType FUTURE;
    static const DXFCPP_EXPORT InstrumentProfileType OPTION;
    static const DXFCPP_EXPORT InstrumentProfileType WARRANT;
    static const DXFCPP_EXPORT InstrumentProfileType CFD;
    static const DXFCPP_EXPORT InstrumentProfileType SPREAD;
    static const DXFCPP_EXPORT InstrumentProfileType OTHER;
    static const DXFCPP_EXPORT InstrumentProfileType REMOVED;
};

template <>
const std::unordered_map<InstrumentProfileType::CodeType, std::reference_wrapper<const InstrumentProfileType>>
    InstrumentProfileType::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()