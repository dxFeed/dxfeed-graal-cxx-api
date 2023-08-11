// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "CandleAlignment.hpp"
#include "CandleExchange.hpp"
#include "CandlePeriod.hpp"
#include "CandlePrice.hpp"
#include "CandlePriceLevel.hpp"
#include "CandleSession.hpp"
#include "CandleSymbolAttribute.hpp"

#include <variant>

namespace dxfcpp {

class DXFCPP_EXPORT CandleSymbol {
    using CandleSymbolAttributeT =
        std::variant<CandleExchange, CandlePrice, CandleSession, CandlePeriod, CandleAlignment, CandlePriceLevel>;

    std::string symbol_;
    std::string baseSymbol_;
    std::unordered_map<CandleSymbolAttributeType, CandleSymbolAttributeT> attributes_;
};

} // namespace dxfcpp