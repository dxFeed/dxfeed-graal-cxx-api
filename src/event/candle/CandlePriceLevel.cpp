// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePriceLevel.hpp"

DXFCPP_BEGIN_NAMESPACE

CandlePriceLevel::CandlePriceLevel(double value) noexcept : value_{value} {
}

CandlePriceLevel::CandlePriceLevel() noexcept {
}

CandlePriceLevel::~CandlePriceLevel() noexcept {
}

double CandlePriceLevel::getValue() const noexcept {
    return value_;
}

std::string CandlePriceLevel::toString() const {
    return math::isInt64(value_) ? std::to_string(static_cast<std::int64_t>(value_)) : dxfcpp::toString(value_);
}

bool CandlePriceLevel::operator==(const CandlePriceLevel &candlePriceLevel) const noexcept {
    return math::doubleEquals(value_, candlePriceLevel.getValue());
}

std::string CandlePriceLevel::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
}

CandlePriceLevel CandlePriceLevel::parse(const StringLike &s) {
    const auto string = std::string{s};
    std::size_t parsedLength = 0;

    try {
        const auto value = std::stod(string, &parsedLength);

        if (parsedLength != string.size()) {
            throw InvalidArgumentException("Incorrect candle price level: " + string);
        }

        return valueOf(value);
    } catch (const InvalidArgumentException &) {
        throw;
    } catch (const std::exception &) {
        throw InvalidArgumentException("Incorrect candle price level: " + string);
    }
}

CandlePriceLevel CandlePriceLevel::valueOf(double value) {
    if (std::isinf(value) || (!std::isnan(value) && std::signbit(value))) {
        throw InvalidArgumentException("Incorrect candle price level: " + dxfcpp::toString(value));
    }

    return std::isnan(value) ? DEFAULT : CandlePriceLevel(value);
}

CandlePriceLevel CandlePriceLevel::getAttributeForSymbol(const StringLike &symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    return !stringOpt ? DEFAULT : parse(stringOpt.value());
}

std::string CandlePriceLevel::normalizeAttributeForSymbol(const StringLike &symbol) {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    if (!a) {
        return symbol;
    }

    try {
        const auto other = parse(a.value());

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
DXFCPP_END_NAMESPACE
