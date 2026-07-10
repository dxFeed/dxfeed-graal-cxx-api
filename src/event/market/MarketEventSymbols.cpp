// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketEventSymbols.hpp"

DXFCPP_BEGIN_NAMESPACE

bool MarketEventSymbols::hasExchangeCode(const StringLike &symbol) noexcept {
    return hasExchangeCodeInternal(symbol, getLengthWithoutAttributesInternal(symbol));
}

char MarketEventSymbols::getExchangeCode(const StringLike &symbol) noexcept {
    return hasExchangeCode(symbol) ? std::string_view(symbol)[getLengthWithoutAttributesInternal(symbol) - 1] : '\0';
}

std::string MarketEventSymbols::changeExchangeCode(const StringLike &symbol, char exchangeCode) noexcept {
    const auto lengthWithoutAttributes = getLengthWithoutAttributesInternal(symbol);
    std::string result;

    if (exchangeCode == '\0') {
        result = getBaseSymbolInternal(symbol, lengthWithoutAttributes);
    } else {
        result = getBaseSymbolInternal(symbol, lengthWithoutAttributes) + EXCHANGE_SEPARATOR + exchangeCode;
    }

    return lengthWithoutAttributes == symbol.length() ? result : result + symbol.substr(lengthWithoutAttributes);
}

std::string MarketEventSymbols::getBaseSymbol(const StringLike &symbol) noexcept {
    return getBaseSymbolInternal(symbol, getLengthWithoutAttributesInternal(symbol));
}

std::string MarketEventSymbols::changeBaseSymbol(const StringLike &symbol, const StringLike &baseSymbol) noexcept {
    const auto lengthWithoutAttributes = getLengthWithoutAttributesInternal(symbol);

    if (hasExchangeCodeInternal(symbol, lengthWithoutAttributes)) {
        return std::string(baseSymbol) + EXCHANGE_SEPARATOR + std::string_view(symbol)[lengthWithoutAttributes - 1] +
               symbol.substr(lengthWithoutAttributes);
    }

    if (lengthWithoutAttributes == symbol.length()) {
        return std::string(baseSymbol);
    }

    return std::string(baseSymbol) + symbol.substr(lengthWithoutAttributes);
}

std::optional<std::string> MarketEventSymbols::getAttributeStringByKey(const StringLike &symbol,
                                                                       const StringLike &key) noexcept {
    return getAttributeInternal(symbol, getLengthWithoutAttributesInternal(symbol), key);
}

std::string MarketEventSymbols::changeAttributeStringByKey(const StringLike &symbol, const StringLike &key,
                                                           const StringLike &value) noexcept {
    const auto lengthWithoutAttributes = getLengthWithoutAttributesInternal(symbol);

    if (lengthWithoutAttributes == symbol.length()) {
        return std::string(symbol) + ATTRIBUTES_OPEN + key + ATTRIBUTE_VALUE + value + ATTRIBUTES_CLOSE;
    }

    return addAttributeInternal(symbol, lengthWithoutAttributes, key, value);
}

std::string MarketEventSymbols::removeAttributeStringByKey(const StringLike &symbol, const StringLike &key) noexcept {
    return removeAttributeInternal(symbol, getLengthWithoutAttributesInternal(symbol), key);
}

bool MarketEventSymbols::hasExchangeCodeInternal(const std::string &symbol, std::size_t length) noexcept {
    return length >= 2 && symbol[length - 2] == EXCHANGE_SEPARATOR;
}

std::string MarketEventSymbols::getBaseSymbolInternal(const std::string &symbol, std::size_t length) noexcept {
    if (hasExchangeCodeInternal(symbol, length)) {
        return symbol.substr(0, length - 2);
    }

    return symbol.substr(0, length);
}

bool MarketEventSymbols::hasAttributesInternal(const std::string &symbol) noexcept {
    if (symbol.length() >= 3 /* ATTRIBUTES_OPEN + ATTRIBUTES_CLOSE + ATTRIBUTE */ &&
        symbol[symbol.length() - 1] == ATTRIBUTES_CLOSE) {
        const auto attributesOpenPos = symbol.find_last_of(ATTRIBUTES_OPEN, symbol.length() - 2);

        return attributesOpenPos != std::string::npos && attributesOpenPos < symbol.length() - 2;
    }

    return false;
}

std::size_t MarketEventSymbols::getLengthWithoutAttributesInternal(const std::string &symbol) noexcept {
    if (hasAttributesInternal(symbol)) {
        return symbol.find_last_of(ATTRIBUTES_OPEN);
    }

    return symbol.length();
}

std::optional<std::string> MarketEventSymbols::getKeyInternal(const std::string &symbol, std::size_t i) noexcept {
    try {
        if (const auto found = symbol.find_first_of(ATTRIBUTE_VALUE, i); found != std::string::npos) {
            return symbol.substr(i, found - i);
        }

        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::size_t MarketEventSymbols::getNextKeyInternal(const std::string &symbol, std::size_t i) noexcept {
    const auto valuePos = symbol.find_first_of(ATTRIBUTE_VALUE, i) + 1;
    const auto separatorPos = symbol.find_first_of(ATTRIBUTES_SEPARATOR, valuePos);

    return separatorPos == std::string::npos ? symbol.length() : separatorPos + 1;
}

std::string MarketEventSymbols::getValueInternal(const std::string &symbol, std::size_t i, std::size_t j) {
    const auto valueOffset = symbol.find_first_of(ATTRIBUTE_VALUE, i) + 1;

    return symbol.substr(valueOffset, j - 1 - valueOffset);
}

std::string MarketEventSymbols::dropKeyAndValueInternal(const std::string &symbol, std::size_t length, std::size_t i,
                                                        std::size_t j) noexcept {
    try {
        if (j == symbol.length()) {
            if (i == length + 1) {
                return symbol.substr(0, length);
            }

            return symbol.substr(0, i - 1) + symbol.substr(j - 1);
        }

        return symbol.substr(0, i) + symbol.substr(j);
    } catch (...) {
        return symbol;
    }
}
std::optional<std::string> MarketEventSymbols::getAttributeInternal(const std::string &symbol,
                                                                    std::size_t lengthWithoutAttributes,
                                                                    const std::string &key) {
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

std::string MarketEventSymbols::removeAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes,
                                                        const std::string &key) noexcept {
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

std::string MarketEventSymbols::addAttributeInternal(std::string symbol, std::size_t lengthWithoutAttributes,
                                                     const std::string &key, const std::string &value) {
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

        const auto j = getNextKeyInternal(symbol, i);

        if (const auto cmp = currentKey.value().compare(key); cmp == 0) {
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

    if (added) {
        return symbol;
    }

    return symbol.substr(0, i - 1) + ATTRIBUTES_SEPARATOR + key + ATTRIBUTE_VALUE + value + symbol.substr(i - 1);
}

DXFCPP_END_NAMESPACE