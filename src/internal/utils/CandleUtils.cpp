// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "CandleUtils.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketEventSymbols.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace candle {

std::string removeDefaultAttribute(const StringLike &symbol, const StringLike &key, const StringLike &defaultValue) {
    auto removed = MarketEventSymbols::removeAttributeStringByKey(symbol, key);

    if (MarketEventSymbols::getBaseSymbol(removed) == MarketEventSymbols::getBaseSymbol(symbol) &&
        MarketEventSymbols::getExchangeCode(removed) == MarketEventSymbols::getExchangeCode(symbol)) {
        return removed;
    }

    return MarketEventSymbols::changeAttributeStringByKey(symbol, key, defaultValue);
}

} // namespace candle

DXFCPP_END_NAMESPACE
