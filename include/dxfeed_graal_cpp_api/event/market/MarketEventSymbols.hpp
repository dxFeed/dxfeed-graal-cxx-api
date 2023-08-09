// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

namespace dxfcpp {

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
     * Removes one attribute with the specified key while leaving exchange code and other attributes intact.
     *
     * @param symbol old symbol.
     * @param key attribute key.
     * @return new symbol without the specified key and everything else from the old symbol.
     */
    static constexpr std::string removeAttributeStringByKey(const std::string &symbol,
                                                            const std::string &key) noexcept {
        return removeAttributeInternal(symbol, getLengthWithoutAttributesInternal(symbol), key);
    }

  private:
    static constexpr char ATTRIBUTES_OPEN = '{';
    static constexpr char ATTRIBUTES_CLOSE = '}';
    static constexpr char ATTRIBUTES_SEPARATOR = ',';
    static constexpr char ATTRIBUTE_VALUE = '=';

    static constexpr bool hasAttributesInternal(const std::string &symbol) noexcept {
        if (symbol.length() >= 3 /* ATTRIBUTES_OPEN + ATTRIBUTES_CLOSE + ATTRIBUTE */ &&
            symbol[symbol.length() - 1] == ATTRIBUTES_CLOSE) {
            auto attributesOpenPos = symbol.find_last_of(ATTRIBUTES_OPEN, symbol.length() - 2);

            return attributesOpenPos != std::string::npos && attributesOpenPos < symbol.length() - 2;
        }

        return false;
    }

    static constexpr std::size_t getLengthWithoutAttributesInternal(const std::string &symbol) noexcept {
        return hasAttributesInternal(symbol) ? symbol.find_last_of(ATTRIBUTES_OPEN) : symbol.length();
    }

    static constexpr std::optional<std::string> getKeyInternal(const std::string &symbol, std::size_t i) noexcept {
        if (auto found = symbol.find_first_of(ATTRIBUTE_VALUE, i); found != std::string::npos) {
            return symbol.substr(i, found - i);
        }

        return std::nullopt;
    }

    static constexpr std::size_t getNextKeyInternal(const std::string &symbol, std::size_t i) noexcept {
        auto valuePos = symbol.find_first_of(ATTRIBUTE_VALUE, i) + 1;
        auto separatorPos = symbol.find_first_of(ATTRIBUTES_SEPARATOR, valuePos);

        return separatorPos == std::string::npos ? symbol.length() : separatorPos + 1;
    }

    static constexpr std::string dropKeyAndValueInternal(const std::string &symbol, std::size_t length, std::size_t i,
                                                         std::size_t j) noexcept {
        if (j == symbol.length()) {
            if (i == length + 1) {
                return symbol.substr(0, length);
            } else {
                return symbol.substr(0, i - 1) + symbol.substr(j - 1);
            }
        }

        return symbol.substr(0, i) + symbol.substr(j);
    }

    static constexpr std::string removeAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes,
                                                         const std::string &key) noexcept {
        if (lengthWithoutAttributes == symbol.length()) {
            return symbol;
        }

        std::size_t i = lengthWithoutAttributes + 1;

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
};

} // namespace dxfcpp