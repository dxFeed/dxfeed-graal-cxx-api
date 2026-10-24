// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"
#include "../../internal/utils/StringUtils.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <map>
#include <optional>
#include <string>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Helper class to compose and parse symbols for market events.
 *
 * <h3>Regional symbols</h3>
 *
 * Regional symbol subscription receives events only from a designated exchange, marketplace, or venue
 * instead of receiving composite events from all venues (by default). Regional symbol is composed of a
 * <i>base symbol</i>, ampersand character ('&amp;'), and an exchange code character. For example,
 * <ul>
 * <li>"SPY" is the symbol for composite events for SPDR S&amp;P 500 ETF from all exchanges,
 * <li>"SPY&amp;N" is the symbol for event for SPDR S&amp;P 500 ETF that originates only from the NYSE marketplace.
 * </ul>
 *
 * <h3>Symbol attributes</h3>
 *
 * Market event symbols can have a number of attributes attached to them in curly braces
 * with `<key>=<value>` paris separated by commas. For example,
 * <ul>
 * <li>"SPY{price=bid}" is the market symbol "SPY" with an attribute key "price" set to value "bid".
 * <li>"SPY(=5m,tho=true}" is the market symbol "SPY" with two attributes. One has an empty key and
 * value "5m", while the other has key "tho" and value "true".
 * </ul>
 * The methods in this class always maintain attribute keys in alphabetic order.
 */
struct DXFCPP_EXPORT MarketEventSymbols {
    /**
     * `"[0-9A-Za-z]"`
     *
     * The character class that all exchange codes of regional events match (a regular expression).
     */
    static constexpr char EXCHANGES_PATTERN[] = "[0-9A-Za-z]";

    /**
     * All exchange codes that match MarketEventSymbols::EXCHANGES_PATTERN: the digits and the Latin letters, in the
     * order of their codes.
     */
    static constexpr char SUPPORTED_EXCHANGES[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    /**
     * Returns `true` if the specified symbol has the exchange code specification.
     *
     * @param symbol The symbol.
     * @return `true` if the specified symbol has the exchange code specification.
     */
    static bool hasExchangeCode(const StringLike &symbol) noexcept;

    /**
     * Returns exchange code of the specified symbol or <code>'\0'</code> if none is defined.
     *
     * @param symbol The symbol.
     * @return exchange code of the specified symbol or <code>'\0'</code> if none is defined.
     */
    static char getExchangeCode(const StringLike &symbol) noexcept;

    /**
     * Changes exchange code of the specified symbol or removes it if the new exchange code is <code>'\0'</code>.
     *
     * @param symbol The old symbol.
     * @param exchangeCode The new exchange code.
     * @return new symbol with the changed exchange code.
     */
    static std::string changeExchangeCode(const StringLike &symbol, char exchangeCode) noexcept;

    /**
     * Returns base symbol without exchange code and attributes.
     *
     * @param symbol symbol.
     * @return base symbol without exchange code and attributes.
     */
    static std::string getBaseSymbol(const StringLike &symbol) noexcept;

    /**
     * Changes base symbol while leaving exchange code and attributes intact.
     * @param symbol old symbol.
     * @param baseSymbol new base symbol.
     * @return new symbol with new base symbol and old symbol's exchange code and attributes.
     */
    static std::string changeBaseSymbol(const StringLike &symbol, const StringLike &baseSymbol) noexcept;

    /**
     * Returns value of the attribute with the specified key.
     * The result is `std::nullopt` if the attribute with the specified key is not found.
     *
     * @param symbol symbol.
     * @param key attribute key.
     * @return value of the attribute with the specified key | `std::nullopt` if the attribute with the specified key is
     * not found.
     */
    static std::optional<std::string> getAttributeStringByKey(const StringLike &symbol,
                                                              const StringLike &key) noexcept;

    /**
     * Changes the value of one attribute value while leaving exchange code and other attributes intact.
     *
     * @param symbol old symbol.
     * @param key attribute key.
     * @param value attribute value.
     *
     * @return new symbol with a key attribute with the specified value and everything else from the old symbol.
     */
    static std::string changeAttributeStringByKey(const StringLike &symbol, const StringLike &key,
                                                  const StringLike &value) noexcept;

    /**
     * Removes one attribute with the specified key while leaving exchange code and other attributes intact.
     *
     * @param symbol old symbol.
     * @param key attribute key.
     * @return new symbol without the specified key and everything else from the old symbol.
     */
    static std::string removeAttributeStringByKey(const StringLike &symbol, const StringLike &key) noexcept;

    /**
     * Returns `true` if the specified symbol has any attributes.
     *
     * @param symbol The symbol.
     * @return `true` if the specified symbol has any attributes.
     */
    static bool hasAttributes(const StringLike &symbol) noexcept;

    /**
     * Builds the canonical spread symbol for the specified spread components.
     * The canonical representation uses a special prefix, leg ordering and formatting.
     *
     * Spread legs with a zero ratio are ignored (not included in the spread symbol).
     * If the spread is empty, then an empty string is returned.
     * If the spread consists of one leg with a ratio of 1, then the pure leg symbol is returned.
     * Legs with a NaN ratio, which have no order in Java, go last, by symbol.
     *
     * Example:
     * ```cpp
     * MarketEventSymbols::buildSpreadSymbol({{"AAPL", 1}, {"MSFT", -1}}); // "=AAPL-MSFT"
     * MarketEventSymbols::buildSpreadSymbol({{"A", 0.5}, {"B", -2.5}});   // "=0.5*A-2.5*B"
     * ```
     *
     * @param spread The map of the spread leg symbols to their ratios in the spread.
     * @return The canonical spread symbol for the specified spread components.
     */
    static std::string buildSpreadSymbol(const std::map<std::string, double> &spread);

    /**
     * Returns a string with all exchange codes (the characters with codes 0..127) that match the specified character
     * class. For greater flexibility, the outer square brackets of the character class can be omitted.
     *
     * The character class is an ECMAScript regular expression (`std::regex`): the Java-specific syntax, such as
     * `\p{Upper}` or the `&&` intersection, is not supported.
     *
     * Example:
     * ```cpp
     * MarketEventSymbols::getExchangesByPattern("A-C"); // "ABC"
     * ```
     *
     * @param characterClass The regular expression character class.
     * @return All exchange codes that match the specified character class.
     * @throws InvalidArgumentException if the character class is not a valid regular expression or the matching
     * exchange codes violate MarketEventSymbols::EXCHANGES_PATTERN.
     */
    static std::string getExchangesByPattern(const StringLike &characterClass);

    private:
    static constexpr char EXCHANGE_SEPARATOR = '&';
    static constexpr char ATTRIBUTES_OPEN = '{';
    static constexpr char ATTRIBUTES_CLOSE = '}';
    static constexpr char ATTRIBUTES_SEPARATOR = ',';
    static constexpr char ATTRIBUTE_VALUE = '=';

    static bool hasExchangeCodeInternal(const std::string &symbol, std::size_t length) noexcept;

    static std::string getBaseSymbolInternal(const std::string &symbol, std::size_t length) noexcept;

    static bool hasAttributesInternal(const std::string &symbol) noexcept;

    static std::size_t getLengthWithoutAttributesInternal(const std::string &symbol) noexcept;

    static std::optional<std::string> getKeyInternal(const std::string &symbol, std::size_t i) noexcept;

    static std::size_t getNextKeyInternal(const std::string &symbol, std::size_t i) noexcept;

    static std::string getValueInternal(const std::string &symbol, std::size_t i, std::size_t j);

    static std::string dropKeyAndValueInternal(const std::string &symbol, std::size_t length, std::size_t i,
                                               std::size_t j) noexcept;

    static std::optional<std::string>
    getAttributeInternal(const std::string &symbol, std::size_t lengthWithoutAttributes, const std::string &key);

    static std::string removeAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes,
                                               const std::string &key) noexcept;

    static std::string addAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes,
                                            const std::string &key, const std::string &value);
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()