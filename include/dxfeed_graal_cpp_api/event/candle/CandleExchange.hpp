// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/utils/StringUtils.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>

namespace dxfcpp {

/**
 * Exchange attribute of CandleSymbol defines exchange identifier where data is
 * taken from to build the candles.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getExchangeCode() "MarketEventSymbols.getExchangeCode" and
 * @ref MarketEventSymbols::changeExchangeCode() "changeExchangeCode" methods.
 */
struct DXFCPP_EXPORT CandleExchange : public CandleSymbolAttribute {
    /**
     * Composite exchange where data is taken from all exchanges.
     */
    static const CandleExchange COMPOSITE;

    /**
     * Default exchange is ::COMPOSITE.
     */
    static const CandleExchange DEFAULT;

  private:
    const char exchangeCode_;

    explicit CandleExchange(char exchangeCode) noexcept : exchangeCode_{exchangeCode} {
    }

  public:
    /**
     * Returns exchange code. It is `'\0'` for ::COMPOSITE exchange.
     * @return exchange code.
     */
    char getExchangeCode() const noexcept {
        return exchangeCode_;
    }

    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        // TODO: implement
        //  return MarketEventSymbols::changeExchangeCode(symbol, exchangeCode);
        return std::string();
    }

    /**
     * Returns string representation of this exchange.
     * It is the string `"COMPOSITE"` for ::COMPOSITE exchange or
     * exchange character otherwise.
     *
     * @return string representation of this exchange.
     */
    std::string toString() const noexcept {
        return exchangeCode_ == '\0' ? "COMPOSITE" : std::string(1, exchangeCode_);
    }

    bool operator==(const CandleExchange &candleExchange) const noexcept {
        return exchangeCode_ == candleExchange.exchangeCode_;
    }

    /**
     * Returns exchange attribute object that corresponds to the specified exchange code character.
     *
     * @param exchangeCode The exchange code character.
     * @return exchange attribute object.
     */
    static CandleExchange valueOf(char exchangeCode) noexcept {
        return exchangeCode == '\0' ? COMPOSITE : CandleExchange(exchangeCode);
    }

    /**
     * Returns exchange attribute object of the given candle symbol string.
     * The result is {@link #DEFAULT} if the symbol does not have exchange attribute.
     * @param symbol candle symbol string.
     * @return exchange attribute object of the given candle symbol string.
     */
    static CandleExchange getAttributeForSymbol(const std::string &symbol) noexcept {
        // TODO: implement
        // return valueOf(MarketEventSymbols::getExchangeCode(symbol));
        return COMPOSITE;
    }
};

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::CandleExchange> {
    std::size_t operator()(const dxfcpp::CandleExchange &candleExchange) const noexcept {
        return static_cast<std::size_t>(candleExchange.getExchangeCode());
    }
};