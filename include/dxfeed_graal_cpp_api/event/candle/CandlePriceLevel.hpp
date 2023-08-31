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
 * Candle price level attribute of CandleSymbol defines how candles shall be aggregated in respect to
 * price interval. The negative or infinite values of price interval are treated as exceptional.
 * <ul>
 * <li>Price interval may be equal to zero. It means every unique price creates a particular candle
 * to aggregate all events with this price for the chosen CandlePeriod.
 * <li>Non-zero price level creates sequence of intervals starting from 0:
 * ...,[-pl;0),[0;pl),[pl;2*pl),...,[n*pl,n*pl+pl). Events aggregated by chosen CandlePeriod and price
 * intervals.
 * </ul>
 *
 * <h3>Implementation details</h3>
 * <p>
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via CandlePriceLevel::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandlePriceLevel::toString()
 */
struct DXFCPP_EXPORT CandlePriceLevel : public CandleSymbolAttribute {
    /**
     * Default price level corresponds to `NaN (std::numeric_limits<double>::quiet_NaN())`
     */
    static const CandlePriceLevel DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandlePriceLevel` in a symbol string using methods
     * of MarketEventSymbols class.
     * The value of this constant is "pl".
     * The value that this key shall be set to is equal to the corresponding CandlePriceLevel::toString()
     */
    static const std::string ATTRIBUTE_KEY;

    double value_{};

    explicit CandlePriceLevel(double value) noexcept : value_{value} {
    }

  public:
    CandlePriceLevel() noexcept = default;
    virtual ~CandlePriceLevel() noexcept = default;

    /**
     * Returns price level value. For example, the value of `1` represents `[0;1), [1;2)` and so on intervals
     * to build candles.
     *
     * @return price level value.
     */
    double getValue() const noexcept {
        return value_;
    }

    /**
     * Returns string representation of this price level.
     * The string representation is composed of value.
     * This string representation can be converted back into object with CandlePriceLevel::parse() method.
     *
     * @return string representation of this price level.
     */
    std::string toString() const noexcept {
        if (math::equals(value_, static_cast<std::int64_t>(value_))) {
            return std::to_string(static_cast<std::int64_t>(value_));
        }

        return std::to_string(value_);
    }

    bool operator==(const CandlePriceLevel &candlePriceLevel) const noexcept {
        return math::equals(value_, candlePriceLevel.getValue());
    }

    /**
     * Returns candle event symbol string with this candle price level set.
     *
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this candle price level set.
     */
    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Parses string representation of candle price level into object.
     * Any string that was returned by CandlePriceLevel::toString() can be parsed
     * and case is ignored for parsing.
     *
     * @param s string representation of candle price level.
     * @return candle price level or std::nullopt if the string representation is invalid or value is invalid
     */
    static std::optional<CandlePriceLevel> parse(const std::string &s) noexcept {
        try {
            return valueOf(std::stod(s));
        } catch (...) {
            return std::nullopt;
        }
    }

    /**
     * Returns candle price level with the given value.
     *
     * @param value candle price level value.
     * @return candle price level with the given value and type or std::nullopt if value is invalid
     */
    static std::optional<CandlePriceLevel> valueOf(double value) noexcept {
        if (std::isinf(value) || (value == 0.0 && std::signbit(value))) {
            // TODO: error handling throw IllegalArgumentException("Incorrect candle price level: " + value);

            return std::nullopt;
        }

        return std::isnan(value) ? DEFAULT : CandlePriceLevel(value);
    }

    /**
     * Returns candle price level of the given candle symbol string.
     * The result is CandlePriceLevel::DEFAULT if the symbol does not have candle price level attribute.
     *
     * @param symbol candle symbol string.
     * @return candle price level of the given candle symbol string or std::nullopt if there is no supported attribute's
     */
    static std::optional<CandlePriceLevel> getAttributeForSymbol(const std::string &symbol) noexcept {
        auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !stringOpt ? DEFAULT : parse(stringOpt.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle price level attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle price level attribute.
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

            if (a.value() != other.value().toString()) {
                return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, other.value().toString());
            }
        }

        return symbol;
    }
};

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::CandlePriceLevel> {
    std::size_t operator()(const dxfcpp::CandlePriceLevel &candlePriceLevel) const noexcept {
        return std::hash<double>{}(candlePriceLevel.getValue());
    }
};