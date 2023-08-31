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
#include <vector>

namespace dxfcpp {

/**
 * Price type attribute of CandleSymbol defines price that is used to build the candles.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via CandlePrice::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandlePrice::toString()
 */
struct DXFCPP_EXPORT CandlePrice : public CandleSymbolAttribute {
    /**
     * Last trading price.
     */
    static const CandlePrice LAST;

    /**
     * Quote bid price.
     */
    static const CandlePrice BID;

    /**
     * Quote ask price.
     */
    static const CandlePrice ASK;

    /**
     * Market price defined as average between quote bid and ask prices.
     */
    static const CandlePrice MARK;

    /**
     * Official settlement price that is defined by exchange or last trading price otherwise.
     * It updates based on all PriceType values:
     * PriceType::INDICATIVE, PriceType::PRELIMINARY, and PriceType::FINAL}.
     */
    static const CandlePrice SETTLEMENT;

    /**
     * Default price type is CandlePrice::LAST.
     */
    static const CandlePrice DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandlePrice` in a symbol string using methods of
     * MarketEventSymbols class.
     * The value of this constant is "price".
     * The value that this key shall be set to is equal to the corresponding CandlePrice::toString()
     */
    static const std::string ATTRIBUTE_KEY;

    static const std::unordered_map<std::string, std::reference_wrapper<const CandlePrice>> BY_STRING;
    static const std::vector<std::reference_wrapper<const CandlePrice>> VALUES;

  private:
    std::string string_;

    explicit CandlePrice(std::string string) noexcept : string_{std::move(string)} {
    }

  public:
    CandlePrice() noexcept = default;
    virtual ~CandlePrice() noexcept = default;

    /**
     * Returns candle event symbol string with this candle price type set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this candle price type set.
     */
    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Returns string representation of this candle price type.
     * The string representation of candle price type is a lower case string that corresponds to
     * its @ref CandlePrice::name() "name". For example, CandlePrice::LAST is represented as "last".
     *
     * @return string representation of this candle price type.
     */
    const std::string &toString() const & noexcept {
        return string_;
    }

    bool operator==(const CandlePrice &candlePrice) const noexcept {
        return string_ == candlePrice.string_;
    }

    /**
     * Parses string representation of candle price type into object.
     * Any string that was returned by CandlePrice::toString() can be parsed and case is ignored for parsing.
     *
     * @param s The string representation of candle price type.
     * @return The candle price type (reference) or std::nullopt if there is no supported attribute's value.
     */
    static std::optional<std::reference_wrapper<const CandlePrice>> parse(const std::string &s) noexcept {
        auto n = s.length();

        if (n == 0) {
            // TODO: error handling throw IllegalArgumentException("Missing candle price");

            return std::nullopt;
        }

        auto found = BY_STRING.find(s);

        if (found != BY_STRING.end()) {
            return found->second;
        }

        for (const auto &priceRef : VALUES) {
            const auto &priceStr = priceRef.get().toString();

            if (priceStr.length() >= n && iEquals(priceStr.substr(0, n), s)) {
                return priceRef;
            }
        }

        // TODO: error handling throw IllegalArgumentException("Unknown candle price: " + s);

        return std::nullopt;
    }

    /**
     * Returns candle price type of the given candle symbol string.
     * The result is CandlePrice::DEFAULT if the symbol does not have candle price type attribute or std::nullopt if
     * there is no supported attribute's value.
     *
     * @param symbol The candle symbol string.
     * @return candle price type of the given candle symbol string or std::nullopt if there is no supported attribute's
     * value.
     */
    static std::optional<std::reference_wrapper<const CandlePrice>>
    getAttributeForSymbol(const std::string &symbol) noexcept {
        auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !stringOpt ? DEFAULT : parse(stringOpt.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle price type attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle price type attribute.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string normalizeAttributeForSymbol(const std::string &symbol) noexcept {
        auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        if (!a) {
            return symbol;
        }

        auto other = parse(a.value());

        if (other.has_value()) {
            if (other.value() == DEFAULT) {
                return MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY);
            }

            if (a.value() != other.value().get().toString()) {
                return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY,
                                                                      other.value().get().toString());
            }
        }

        return symbol;
    }
};

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::CandlePrice> {
    std::size_t operator()(const dxfcpp::CandlePrice &candlePrice) const noexcept {
        return std::hash<std::string>{}(candlePrice.toString());
    }
};