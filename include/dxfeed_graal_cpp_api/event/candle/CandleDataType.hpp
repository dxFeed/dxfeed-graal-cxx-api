// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/utils/StringUtils.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "./CandleSymbolAttribute.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

/**
 * \addtogroup dxfcpp_candle
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Candle data type attribute of CandleSymbol defines which types of data shall be used for candles aggregation.
 * This attribute is designed as an open-ended replacement for CandleSession attribute: besides the constants of this
 * class, any string is a valid data type.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via CandleDataType::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandleDataType::toString()
 */
struct DXFCPP_EXPORT CandleDataType final : CandleSymbolAttribute {
    /**
     * Aggregation rules are defined per data source (e.g., an exchange).
     */
    static const CandleDataType SOURCE;

    /**
     * Selected types of data are identical to those used for CandleSession::REGULAR.
     */
    static const CandleDataType THO;

    /**
     * All valid TimeAndSales or Quotes are used to build candles.
     */
    static const CandleDataType ALL;

    /**
     * All valid non-ETH TimeAndSales or Quotes from a regular trading hours are used to build candles.
     */
    static const CandleDataType RTH;

    /**
     * All valid ETH TimeAndSales or Quotes from an extended trading hours are used to build candles.
     */
    static const CandleDataType ETH;

    /**
     * Data type is selected by a chart server.
     */
    static const CandleDataType UNDEFINED;

    /**
     * Default data type is CandleDataType::UNDEFINED.
     */
    static const CandleDataType DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandleDataType` in a symbol string using methods of
     * MarketEventSymbols class.
     * The value of this constant is "dt", which is an abbreviation for "data type".
     * The value that this key shall be set to is equal to the corresponding CandleDataType::toString()
     */
    static constexpr char ATTRIBUTE_KEY[] = "dt";

    private:
    // The constants refer to literals, other data types to the copies of their values in a table of the library, so
    // the type is trivially destructible and the constants can be used by the destructors of global variables.
    std::string_view value_{};

    constexpr explicit CandleDataType(std::string_view value) noexcept : value_{value} {
    }

    public:
    /// Creates the CandleDataType::UNDEFINED data type.
    CandleDataType() noexcept;

    /**
     * Returns string representation of this data type attribute.
     * Typically, the string representation of candle data type attribute is a lower case string that corresponds to
     * its name. For example, CandleDataType::THO is represented as "tho".
     *
     * @return string representation of this candle data type attribute.
     */
    std::string getValue() const;

    /**
     * Returns string representation of this data type attribute (the same as CandleDataType::getValue()).
     * This string representation can be converted back into an object with the CandleDataType::parse() method.
     *
     * @return string representation of this candle data type attribute.
     */
    std::string toString() const;

    /**
     * Compares the values of the data types (case-sensitive).
     *
     * @param candleDataType The other data type.
     * @return `true` if the values are equal.
     */
    bool operator==(const CandleDataType &candleDataType) const noexcept;

    /**
     * Returns candle event symbol string with this data type attribute set.
     * CandleDataType::DEFAULT removes the attribute.
     *
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this data type attribute set.
     */
    std::string changeAttributeForSymbol(const StringLike &symbol) const override;

    /**
     * Parses string representation of candle data type attribute into an object (the same as
     * CandleDataType::valueOf()). Any string that was returned by CandleDataType::toString() can be parsed.
     *
     * @param s string representation of a candle data type attribute.
     * @return candle data type attribute.
     */
    static CandleDataType parse(const StringLike &s);

    /**
     * Returns the candle data type with the given value. The value is case-sensitive: the constant if it is equal to
     * the value of a constant, CandleDataType::DEFAULT for an empty string, otherwise a new data type with this value.
     *
     * A new data type refers to a copy of its value that the library keeps (one per distinct value) until the program
     * exits, so it must not be used by the destructor of a global variable that was constructed before the first
     * data type with this value.
     *
     * @param value string representation of a candle data type attribute.
     * @return candle data type attribute.
     */
    static CandleDataType valueOf(const StringLike &value);

    /**
     * Returns candle data type attribute of the given candle symbol string.
     * The result is CandleDataType::DEFAULT if the symbol does not have the candle data type attribute.
     *
     * @param symbol candle symbol string.
     * @return candle data type attribute of the given candle symbol string.
     */
    static CandleDataType getAttributeForSymbol(const StringLike &symbol);

    /**
     * Returns candle symbol string with the normalized representation of the candle data type attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the candle data type attribute.
     */
    static std::string normalizeAttributeForSymbol(const StringLike &symbol);
};

DXFCPP_END_NAMESPACE

/// @}

template <> struct std::hash<dxfcpp::CandleDataType> {
    std::size_t operator()(const dxfcpp::CandleDataType &candleDataType) const noexcept {
        return std::hash<std::string>{}(candleDataType.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
