// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/utils/ConstantMap.hpp"
#include "../../internal/utils/StringUtils.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "./CandleSymbolAttribute.hpp"

#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

/**
 * \addtogroup dxfcpp_candle
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Price type attribute of CandleSymbol defines the price used to build the candles.
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
struct DXFCPP_EXPORT CandlePrice final : CandleSymbolAttribute {
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
     * The default price type is CandlePrice::LAST.
     */
    static const CandlePrice DEFAULT;

    /**
     * The attribute key that is used to store the value of `CandlePrice` in a symbol string using methods of
     * MarketEventSymbols class.
     * The value of this constant is "price".
     * The value that this key shall be set to is equal to the corresponding CandlePrice::toString()
     */
    static constexpr char ATTRIBUTE_KEY[] = "price";

    /// The elements by their string representations (constant-initialized, so it can be used in the dynamic
    /// initialization and the destruction of global variables).
    static const ConstantMap<std::string_view, CandlePrice> BY_STRING;

    /// All elements (constant-initialized, so the initializers of global variables can use it).
    static const std::span<const std::reference_wrapper<const CandlePrice>> VALUES;

    private:
    std::string_view string_{};

    constexpr explicit CandlePrice(std::string_view string) noexcept : string_{string} {
    }

    public:
    CandlePrice() noexcept;

    /**
     * Returns candle event symbol string with this candle price type set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this candle price type set.
     */
    std::string changeAttributeForSymbol(const StringLike &symbol) const override;

    /**
     * Returns string representation of this candle price type.
     * The string representation of a candle price type is a lower case string that corresponds to
     * its name. For example, CandlePrice::LAST is represented as "last".
     *
     * @return string representation of this candle price type.
     */
    std::string toString() const;

    bool operator==(const CandlePrice &candlePrice) const noexcept;

    /**
     * Parses string representation of a candle price type into an object.
     * Any string returned by CandlePrice::toString() can be parsed, and a case is ignored for parsing.
     *
     * @param s The string representation of a candle price type.
     * @return The candle price type (reference).
     * @throws InvalidArgumentException if the argument is empty or invalid
     */
    static std::reference_wrapper<const CandlePrice> parse(const StringLike &s);

    /**
     * Returns the candle price type of the given candle symbol string.
     * The result is CandlePrice::DEFAULT if the symbol does not have a candle price type attribute.
     *
     * @param symbol The candle symbol string.
     * @return candle price type of the given candle symbol string.
     */
    static std::reference_wrapper<const CandlePrice> getAttributeForSymbol(const StringLike &symbol);

    /**
     * Returns candle symbol string with the normalized representation of the candle price type attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the candle price type attribute.
     */
    static std::string normalizeAttributeForSymbol(const StringLike &symbol);
};

DXFCPP_END_NAMESPACE

/// @}

template <> struct std::hash<dxfcpp::CandlePrice> {
    std::size_t operator()(const dxfcpp::CandlePrice &candlePrice) const noexcept {
        return std::hash<std::string>{}(candlePrice.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
