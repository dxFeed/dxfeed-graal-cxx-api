// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"
#include "../../exceptions/InvalidArgumentException.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/utils/StringUtils.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"
#include "CandleType.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Period attribute of CandleSymbol defines aggregation period of the candles.
 * Aggregation period is defined as pair of a @ref CandlePeriod::getValue() "value" and @ref CandlePeriod::getType()
 * "type".
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via CandlePeriod::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandlePeriod::toString()
 * "CandlePeriod.toString()"
 */
struct DXFCPP_EXPORT CandlePeriod : public CandleSymbolAttribute {
    /**
     * Tick aggregation where each candle represents an individual tick.
     */
    static const CandlePeriod TICK;

    /**
     * Day aggregation where each candle represents a day.
     */
    static const CandlePeriod DAY;

    /**
     * Default period is CandlePeriod::TICK.
     */
    static const CandlePeriod DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandlePeriod` in a symbol string using methods of
     * MarketEventSymbols class.
     * The value of this constant is an empty string, because this is the main attribute that every CandleSymbol must
     * have.
     * The value that this key shall be set to is equal to the corresponding CandlePeriod::toString()
     */
    static const std::string ATTRIBUTE_KEY; // empty string as attribute key is allowed!

  private:
    static const std::int64_t DEFAULT_PERIOD_VALUE = 1LL;

    double value_{};
    CandleType type_{};
    mutable std::string string_{};

    CandlePeriod(double value, const CandleType &type) noexcept : value_{value}, type_{type}, string_{} {
    }

  public:
    CandlePeriod() noexcept = default;
    virtual ~CandlePeriod() noexcept = default;

    /**
     * Returns aggregation period in milliseconds as closely as possible.
     * Certain aggregation types like @ref CandleType::SECOND "SECOND" and
     * @ref CandleType::DAY "DAY" span a specific number of milliseconds.
     * CandleType::MONTH, CandleType::OPTEXP and CandleType::YEAR are approximate. Candle period of
     * CandleType::TICK, CandleType::VOLUME, CandleType::PRICE, CandleType::PRICE_MOMENTUM and CandleType::PRICE_RENKO
     * is not defined and this method returns `0`.
     * The result of this method is equal to
     * `static_cast<std::int64_t>(this->getType().getPeriodIntervalMillis() * this->getValue())`
     * @see CandleType::getPeriodIntervalMillis()
     * @return aggregation period in milliseconds.
     */
    std::int64_t getPeriodIntervalMillis() const noexcept {
        return static_cast<std::int64_t>(static_cast<double>(type_.getPeriodIntervalMillis()) * value_);
    }

    /**
     * Returns candle event symbol string with this aggregation period set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this aggregation period set.
     */
    std::string changeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) const override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Returns aggregation period value. For example, the value of `5` with
     * the candle type of @ref CandleType::MINUTE "MINUTE" represents `5` minute
     * aggregation period.
     *
     * @return aggregation period value.
     */
    double getValue() const noexcept {
        return value_;
    }

    /**
     * Returns aggregation period type.
     * @return aggregation period type.
     */
    const CandleType &getType() const & noexcept {
        return type_;
    }

    /**
     * Returns string representation of this aggregation period.
     * The string representation is composed of value and type string.
     * For example, 5 minute aggregation is represented as `"5m"`.
     * The value of `1` is omitted in the string representation, so
     * CandlePeriod::DAY (one day) is represented as `"d"`.
     * This string representation can be converted back into object with CandlePeriod::parse() method.
     *
     * @return string representation of this aggregation period.
     */
    const std::string &toString() const & {
        if (string_.empty()) {
            string_ = math::equals(value_, DEFAULT_PERIOD_VALUE) ? type_.toString()
                      : math::equals(value_, static_cast<std::int64_t>(value_))
                          ? std::to_string(static_cast<std::int64_t>(value_)) + "" + type_.toString()
                          : std::to_string(value_) + "" + type_.toString();
        }

        return string_;
    }

    bool operator==(const CandlePeriod &candlePeriod) const {
        return math::equals(value_, candlePeriod.getValue()) && type_ == candlePeriod.getType();
    }

    /**
     * Parses string representation of aggregation period into object.
     * Any string that was returned by CandlePeriod::toString() can be parsed.
     * This method is flexible in the way candle types can be specified.
     * See CandleType::parse() for details.
     *
     * @param s The string representation of aggregation period.
     * @return The aggregation period.
     */
    static CandlePeriod parse(const dxfcpp::StringLikeWrapper &s) {
        if (s == CandleType::DAY.toString()) {
            return DAY;
        }

        if (s == CandleType::TICK.toString()) {
            return TICK;
        }

        auto sw = s.operator std::string_view();
        std::size_t i = 0;
        for (; i < sw.length(); i++) {
            auto c = sw[i];

            if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e' && c != 'E')
                break;
        }

        auto valueStr = sw.substr(0, i);
        auto typeStr = sw.substr(i);
        auto value = valueStr.empty() ? 1.0 : double(s);
        auto type = CandleType::parse(typeStr);

        return valueOf(value, type.get());
    }

    /**
     * Returns candle period with the given value and type.
     *
     * @param value candle period value.
     * @param type candle period type.
     * @return candle period with the given value and type.
     */
    static CandlePeriod valueOf(double value, const CandleType &type) noexcept {
        if (value == 1 && type == CandleType::DAY) {
            return DAY;
        }

        if (value == 1 && type == CandleType::TICK) {
            return TICK;
        }

        return {value, type};
    }

    /**
     * Returns candle period of the given candle symbol string.
     * The result is CandlePeriod::DEFAULT if the symbol does not have candle period attribute.
     *
     * @param symbol candle symbol string.
     * @return candle period of the given candle symbol string
     */
    static CandlePeriod getAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) noexcept {
        auto string = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !string ? DEFAULT : parse(string.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle period attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle period attribute.
     */
    static std::string
    normalizeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
        auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        if (!a) {
            return symbol;
        }

        try {
            auto other = parse(a.value());

            if (other == DEFAULT) {
                return MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY);
            }

            if (a.value() != other.toString()) {
                return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, other.toString());
            }

            return symbol;
        } catch (const InvalidArgumentException &) {
            return symbol;
        } catch (const std::invalid_argument &) {
            return symbol;
        }
    }
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CandlePeriod> {
    std::size_t operator()(const dxfcpp::CandlePeriod &candlePeriod) const noexcept {
        std::size_t seed = 0;

        dxfcpp::hashCombine(seed, candlePeriod.getValue());
        dxfcpp::hashCombine(seed, candlePeriod.getType());

        return seed;
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()