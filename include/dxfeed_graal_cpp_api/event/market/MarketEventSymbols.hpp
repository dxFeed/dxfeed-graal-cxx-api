// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Helper class to compose and parse symbols for market events.
 *
 * <h3>Regional symbols</h3>
 *
 * Regional symbol subscription receives events only from a designated exchange, marketplace, or venue
 * instead of receiving composite events from all venues (by default). Regional symbol is composed from a
 * <i>base symbol</i>, ampersand character ('&amp;'), and an exchange code character. For example,
 * <ul>
 * <li>"SPY" is the symbol for composite events for SPDR S&amp;P 500 ETF from all exchanges,
 * <li>"SPY&amp;N" is the symbol for event for SPDR S&amp;P 500 ETF that originate only from NYSE marketplace.
 * </ul>
 *
 * <h3>Symbol attributes</h3>
 *
 * Market event symbols can have a number of attributes attached to then in curly braces
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
     * Returns `true` is the specified symbol has the exchange code specification.
     *
     * @param symbol The symbol.
     * @return `true` is the specified symbol has the exchange code specification.
     */
    static bool hasExchangeCode(const std::string &symbol) noexcept {
        return hasExchangeCodeInternal(symbol, getLengthWithoutAttributesInternal(symbol));
    }

    /**
     * Returns exchange code of the specified symbol or `'\0'` if none is defined.
     *
     * @param symbol The symbol.
     * @return exchange code of the specified symbol or `'\0'` if none is defined.
     */
    static char getExchangeCode(const std::string &symbol) noexcept {
        return hasExchangeCode(symbol) ? symbol[getLengthWithoutAttributesInternal(symbol) - 1] : '\0';
    }

    /**
     * Changes exchange code of the specified symbol or removes it if new exchange code is `'\0'`.
     *
     * @param symbol The old symbol.
     * @param exchangeCode The new exchange code.
     * @return new symbol with the changed exchange code.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string changeExchangeCode(const std::string &symbol,
                                                                        char exchangeCode) noexcept {
        auto i = getLengthWithoutAttributesInternal(symbol);
        auto result = exchangeCode == '\0' ? getBaseSymbolInternal(symbol, i)
                                           : getBaseSymbolInternal(symbol, i) + EXCHANGE_SEPARATOR + exchangeCode;
        return i == symbol.length() ? result : result + symbol.substr(i);
    }

    /**
     * Returns base symbol without exchange code and attributes.
     *
     * @param symbol symbol.
     * @return base symbol without exchange code and attributes.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string getBaseSymbol(const std::string &symbol) noexcept {
        return getBaseSymbolInternal(symbol, getLengthWithoutAttributesInternal(symbol));
    }

    /**
     * Changes base symbol while leaving exchange code and attributes intact.
     * @param symbol old symbol.
     * @param baseSymbol new base symbol.
     * @return new symbol with new base symbol and old symbol's exchange code and attributes.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string changeBaseSymbol(const std::string &symbol,
                                                                      const std::string &baseSymbol) noexcept {
        auto i = getLengthWithoutAttributesInternal(symbol);

        return hasExchangeCodeInternal(symbol, i) ? baseSymbol + EXCHANGE_SEPARATOR + symbol[i - 1] + symbol.substr(i)
               : i == symbol.length()             ? baseSymbol
                                                  : baseSymbol + symbol.substr(i);
    }

    /**
     * Returns value of the attribute with the specified key.
     * The result is std::nullopt if attribute with the specified key is not found.
     *
     * @param symbol symbol.
     * @param key attribute key.
     * @return value of the attribute with the specified key | std::nullopt if attribute with the specified key is not
     * found.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::optional<std::string>
    getAttributeStringByKey(const std::string &symbol, const std::string &key) noexcept {
        return getAttributeInternal(symbol, getLengthWithoutAttributesInternal(symbol), key);
    }

    /**
     * Changes value of one attribute value while leaving exchange code and other attributes intact.
     *
     * @param symbol old symbol.
     * @param key attribute key.
     * @param value attribute value.
     *
     * @return new symbol with key attribute with the specified value and everything else from the old symbol.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string
    changeAttributeStringByKey(const std::string &symbol, const std::string &key, const std::string &value) noexcept {
        auto i = getLengthWithoutAttributesInternal(symbol);

        if (i == symbol.length())
            return symbol + ATTRIBUTES_OPEN + key + ATTRIBUTE_VALUE + value + ATTRIBUTES_CLOSE;
        return addAttributeInternal(symbol, i, key, value);
    }

    /**
     * Removes one attribute with the specified key while leaving exchange code and other attributes intact.
     *
     * @param symbol old symbol.
     * @param key attribute key.
     * @return new symbol without the specified key and everything else from the old symbol.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string removeAttributeStringByKey(const std::string &symbol,
                                                                                const std::string &key) noexcept {
        return removeAttributeInternal(symbol, getLengthWithoutAttributesInternal(symbol), key);
    }

  private:
    static constexpr char EXCHANGE_SEPARATOR = '&';
    static constexpr char ATTRIBUTES_OPEN = '{';
    static constexpr char ATTRIBUTES_CLOSE = '}';
    static constexpr char ATTRIBUTES_SEPARATOR = ',';
    static constexpr char ATTRIBUTE_VALUE = '=';

    static constexpr bool hasExchangeCodeInternal(const std::string &symbol, std::size_t length) noexcept {
        return length >= 2 && symbol[length - 2] == EXCHANGE_SEPARATOR;
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::string getBaseSymbolInternal(const std::string &symbol,
                                                                           std::size_t length) noexcept {
        return hasExchangeCodeInternal(symbol, length) ? symbol.substr(0, length - 2) : symbol.substr(0, length);
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING bool hasAttributesInternal(const std::string &symbol) noexcept {
        if (symbol.length() >= 3 /* ATTRIBUTES_OPEN + ATTRIBUTES_CLOSE + ATTRIBUTE */ &&
            symbol[symbol.length() - 1] == ATTRIBUTES_CLOSE) {
            auto attributesOpenPos = symbol.find_last_of(ATTRIBUTES_OPEN, symbol.length() - 2);

            return attributesOpenPos != std::string::npos && attributesOpenPos < symbol.length() - 2;
        }

        return false;
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::size_t
    getLengthWithoutAttributesInternal(const std::string &symbol) noexcept {
        return hasAttributesInternal(symbol) ? symbol.find_last_of(ATTRIBUTES_OPEN) : symbol.length();
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::optional<std::string> getKeyInternal(const std::string &symbol,
                                                                                   std::size_t i) noexcept {
        try {
            if (auto found = symbol.find_first_of(ATTRIBUTE_VALUE, i); found != std::string::npos) {
                return symbol.substr(i, found - i);
            }

            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::size_t getNextKeyInternal(const std::string &symbol,
                                                                        std::size_t i) noexcept {
        auto valuePos = symbol.find_first_of(ATTRIBUTE_VALUE, i) + 1;
        auto separatorPos = symbol.find_first_of(ATTRIBUTES_SEPARATOR, valuePos);

        return separatorPos == std::string::npos ? symbol.length() : separatorPos + 1;
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::string getValueInternal(const std::string &symbol, std::size_t i,
                                                                      std::size_t j) {
        auto valueOffset = symbol.find_first_of(ATTRIBUTE_VALUE, i) + 1;

        return symbol.substr(valueOffset, j - 1 - valueOffset);
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::string
    dropKeyAndValueInternal(const std::string &symbol, std::size_t length, std::size_t i, std::size_t j) noexcept {
        try {
            if (j == symbol.length()) {
                if (i == length + 1) {
                    return symbol.substr(0, length);
                } else {
                    return symbol.substr(0, i - 1) + symbol.substr(j - 1);
                }
            }

            return symbol.substr(0, i) + symbol.substr(j);
        } catch (...) {
            return symbol;
        }
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::optional<std::string>
    getAttributeInternal(const std::string &symbol, std::size_t lengthWithoutAttributes, const std::string &key) {
        if (lengthWithoutAttributes == symbol.length()) {
            return std::nullopt;
        }

        auto i = lengthWithoutAttributes + 1;

        while (i < symbol.length()) {
            auto currentKey = getKeyInternal(symbol, i);

            if (!currentKey) {
                break;
            }

            auto j = getNextKeyInternal(symbol, i);

            if (key == currentKey.value()) {
                return getValueInternal(symbol, i, j);
            }

            i = j;
        }

        return std::nullopt;
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::string
    removeAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes, const std::string &key) noexcept {
        if (lengthWithoutAttributes == symbol.length()) {
            return symbol;
        }

        auto i = lengthWithoutAttributes + 1;

        while (i < symbol.length()) {
            auto currentKey = getKeyInternal(symbol, i);

            if (!currentKey) {
                break;
            }

            auto j = getNextKeyInternal(symbol, i);

            if (key == currentKey.value()) {
                symbol = dropKeyAndValueInternal(symbol, lengthWithoutAttributes, i, j);
            } else {
                i = j;
            }
        }

        return symbol;
    }

    static DXFCPP_CXX20_CONSTEXPR_STRING std::string addAttributeInternal(std::string symbol,
                                                                          std::size_t lengthWithoutAttributes,
                                                                          const std::string &key,
                                                                          const std::string &value) {
        if (lengthWithoutAttributes == symbol.length()) {
            return symbol + ATTRIBUTES_OPEN + key + ATTRIBUTE_VALUE + value + ATTRIBUTES_CLOSE;
        }

        auto i = lengthWithoutAttributes + 1;
        bool added = false;

        while (i < symbol.length()) {
            auto currentKey = getKeyInternal(symbol, i);

            if (!currentKey) {
                break;
            }

            auto j = getNextKeyInternal(symbol, i);
            auto cmp = currentKey.value().compare(key);

            if (cmp == 0) {
                if (added) {
                    // drop, since we've already added this key
                    symbol = dropKeyAndValueInternal(symbol, lengthWithoutAttributes, i, j);
                } else {
                    // replace value
                    symbol = symbol.substr(0, i) + key + ATTRIBUTE_VALUE + value + symbol.substr(j - 1);
                    added = true;
                    i += key.length() + value.length() + 2;
                }
            } else if (cmp > 0 && !added) {
                // insert value here
                symbol = symbol.substr(0, i) + key + ATTRIBUTE_VALUE + value + ATTRIBUTES_SEPARATOR + symbol.substr(i);
                added = true;
                i += key.length() + value.length() + 2;
            } else {
                i = j;
            }
        }

        return added ? symbol
                     : symbol.substr(0, i - 1) + ATTRIBUTES_SEPARATOR + key + ATTRIBUTE_VALUE + value +
                           symbol.substr(i - 1);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()