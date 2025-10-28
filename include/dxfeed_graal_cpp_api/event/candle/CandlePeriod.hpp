// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"
#include "CandleType.hpp"
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Period attribute of CandleSymbol defines an aggregation period of the candles.
 * Aggregation period is defined as a pair of a @ref CandlePeriod::getValue() "value" and @ref CandlePeriod::getType()
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
     * The default period is CandlePeriod::TICK.
     */
    static const CandlePeriod DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandlePeriod` in a symbol string using methods of
     * MarketEventSymbols class.
     * The value of this constant is an empty string because this is the main attribute that every CandleSymbol must
     * have.
     * The value that this key shall be set to is equal to the corresponding CandlePeriod::toString()
     */
    static const std::string ATTRIBUTE_KEY; // empty string as an attribute key is allowed!

    private:
    static const std::int64_t DEFAULT_PERIOD_VALUE = 1LL;

    double value_{};
    CandleType type_{};
    mutable std::string string_{};

    CandlePeriod(double value, const CandleType &type) noexcept;

    public:
    CandlePeriod() noexcept = default;
    virtual ~CandlePeriod() noexcept = default;

    /**
     * Returns an aggregation period in milliseconds as closely as possible.
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
    std::int64_t getPeriodIntervalMillis() const noexcept;

    /**
     * Returns candle event symbol string with this aggregation period set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this aggregation period set.
     */
    std::string changeAttributeForSymbol(const StringLike &symbol) const override;

    /**
     * Returns aggregation period value. For example, the value of `5` with
     * the candle type of @ref CandleType::MINUTE "MINUTE" represents `5` minute
     * aggregation period.
     *
     * @return aggregation period value.
     */
    double getValue() const noexcept;

    /**
     * Returns aggregation period type.
     * @return aggregation period type.
     */
    const CandleType &getType() const & noexcept;

    /**
     * Returns string representation of this aggregation period.
     * The string representation is composed of value and type string.
     * For example, 5-minute aggregation is represented as `"5m"`.
     * The value of `1` is omitted in the string representation, so
     * CandlePeriod::DAY (one day) is represented as `"d"`.
     * This string representation can be converted back into an object with the CandlePeriod::parse() method.
     *
     * @return string representation of this aggregation period.
     */
    const std::string &toString() const &;

    bool operator==(const CandlePeriod &candlePeriod) const;

    /**
     * Parses string representation of an aggregation period into an object.
     * Any string returned by CandlePeriod::toString() can be parsed.
     * This method is flexible in the way candle types can be specified.
     * See CandleType::parse() for details.
     *
     * @param s The string representation of an aggregation period.
     * @return The aggregation period.
     */
    static CandlePeriod parse(const StringLike &s);

    /**
     * Returns candle period with the given value and type.
     *
     * @param value candle period value.
     * @param type candle period type.
     * @return candle period with the given value and type.
     */
    static CandlePeriod valueOf(double value, const CandleType &type) noexcept;

    /**
     * Returns candle period of the given candle symbol string.
     * The result is CandlePeriod::DEFAULT if the symbol does not have a candle period attribute.
     *
     * @param symbol candle symbol string.
     * @return candle period of the given candle symbol string
     */
    static CandlePeriod getAttributeForSymbol(const StringLike &symbol) noexcept;

    /**
     * Returns candle symbol string with the normalized representation of the candle period attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the candle period attribute.
     */
    static std::string normalizeAttributeForSymbol(const StringLike &symbol);
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