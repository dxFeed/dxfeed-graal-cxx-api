// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleExchange.hpp"

DXFCPP_BEGIN_NAMESPACE

const CandleExchange CandleExchange::COMPOSITE{'\0'};

const CandleExchange CandleExchange::DEFAULT = COMPOSITE;

CandleExchange::CandleExchange(char exchangeCode) noexcept : exchangeCode_{exchangeCode} {
}

char CandleExchange::getExchangeCode() const noexcept {
    return exchangeCode_;
}

std::string CandleExchange::changeAttributeForSymbol(const StringLike &symbol) const {
    return MarketEventSymbols::changeExchangeCode(symbol, exchangeCode_);
}

std::string CandleExchange::toString() const {
    return exchangeCode_ == '\0' ? "COMPOSITE" : std::string(1, exchangeCode_);
}

bool CandleExchange::operator==(const CandleExchange &candleExchange) const noexcept {
    return exchangeCode_ == candleExchange.exchangeCode_;
}

CandleExchange CandleExchange::valueOf(char exchangeCode) noexcept {
    return exchangeCode == '\0' ? COMPOSITE : CandleExchange(exchangeCode);
}

CandleExchange CandleExchange::getAttributeForSymbol(const StringLike &symbol) {
    return valueOf(MarketEventSymbols::getExchangeCode(symbol));
}

DXFCPP_END_NAMESPACE