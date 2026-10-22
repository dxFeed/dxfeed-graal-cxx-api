// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePriceLevel.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include "../../internal/utils/CandleUtils.hpp"
#include "../../internal/utils/JavaUtils.hpp"

#include <cmath>

DXFCPP_BEGIN_NAMESPACE

CandlePriceLevel::CandlePriceLevel() noexcept {
}

double CandlePriceLevel::getValue() const noexcept {
    return value_;
}

std::string CandlePriceLevel::toString() const {
    return java::longOrDoubleToString(value_);
}

bool CandlePriceLevel::operator==(const CandlePriceLevel &candlePriceLevel) const noexcept {
    return math::doubleEquals(value_, candlePriceLevel.getValue());
}

std::string CandlePriceLevel::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT
               ? MarketEventSymbols::removeAttributeStringByKey(symbol, CandlePriceLevel::ATTRIBUTE_KEY)
               : MarketEventSymbols::changeAttributeStringByKey(symbol, CandlePriceLevel::ATTRIBUTE_KEY, toString());
}

CandlePriceLevel CandlePriceLevel::parse(const StringLike &s) {
    const auto value = java::parseDouble(s.operator std::string_view());

    if (!value) {
        throw InvalidArgumentException("Incorrect candle price level: " + std::string(s));
    }

    return valueOf(*value);
}

CandlePriceLevel CandlePriceLevel::valueOf(double value) {
    if (std::isinf(value) || (!std::isnan(value) && std::signbit(value))) {
        throw InvalidArgumentException("Incorrect candle price level: " + java::doubleToString(value));
    }

    return std::isnan(value) ? DEFAULT : CandlePriceLevel(value);
}

CandlePriceLevel CandlePriceLevel::getAttributeForSymbol(const StringLike &symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, CandlePriceLevel::ATTRIBUTE_KEY);

    return !stringOpt ? DEFAULT : parse(stringOpt.value());
}

std::string CandlePriceLevel::normalizeAttributeForSymbol(const StringLike &symbol) {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, CandlePriceLevel::ATTRIBUTE_KEY);

    if (!a) {
        return symbol;
    }

    try {
        const auto other = parse(a.value());

        if (other == DEFAULT) {
            return candle::removeDefaultAttribute(symbol, CandlePriceLevel::ATTRIBUTE_KEY, DEFAULT.toString());
        }

        if (a.value() != other.toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, CandlePriceLevel::ATTRIBUTE_KEY,
                                                                  other.toString());
        }

        return symbol;
    } catch (const InvalidArgumentException &) {
        return symbol;
    } catch (const std::invalid_argument &) {
        return symbol;
    }
}
DXFCPP_END_NAMESPACE
