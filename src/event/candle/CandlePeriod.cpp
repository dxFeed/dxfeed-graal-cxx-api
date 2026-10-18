// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePeriod.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleType.hpp"

#include "../../internal/utils/JavaUtils.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace {

// Java CandlePeriod.toString(): the type alone for the value 1, a long when the value equals its (long) cast,
// otherwise Double.toString of the value.
std::string toJavaString(double value, const CandleType &type) {
    if (math::exactlyEquals(value, 1.0)) {
        return type.toString();
    }

    const auto longValue = java::longCast(value);

    if (math::exactlyEquals(value, static_cast<double>(longValue))) {
        return std::to_string(longValue) + type.toString();
    }

    return java::doubleToString(value) + type.toString();
}

} // namespace

CandlePeriod::CandlePeriod(double value, const CandleType &type)
    : value_{value}, type_{type}, string_{toJavaString(value_, type_)} {
}

CandlePeriod::CandlePeriod() : CandlePeriod(0.0, CandleType{}) {
}

CandlePeriod::~CandlePeriod() noexcept {
}

std::int64_t CandlePeriod::getPeriodIntervalMillis() const noexcept {
    return java::longCast(static_cast<double>(type_.getPeriodIntervalMillis()) * value_);
}

std::string CandlePeriod::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
}

double CandlePeriod::getValue() const noexcept {
    return value_;
}

const CandleType &CandlePeriod::getType() const & noexcept {
    return type_;
}

const std::string &CandlePeriod::toString() const & {
    return string_;
}

bool CandlePeriod::operator==(const CandlePeriod &candlePeriod) const {
    return math::doubleEquals(value_, candlePeriod.getValue()) && type_ == candlePeriod.getType();
}

CandlePeriod CandlePeriod::parse(const StringLike &s) {
    if (s == CandleType::DAY.toString()) {
        return DAY;
    }

    if (s == CandleType::TICK.toString()) {
        return TICK;
    }

    const auto sw = s.operator std::string_view();
    std::size_t i = 0;

    for (; i < sw.length(); i++) {
        auto c = sw[i];

        if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e' && c != 'E')
            break;
    }

    const auto valueStr = sw.substr(0, i);
    const auto typeStr = sw.substr(i);
    const auto value = valueStr.empty() ? std::optional<double>{1.0} : java::parseDouble(valueStr);

    if (!value) {
        throw InvalidArgumentException("Invalid candle period value: " + std::string(valueStr));
    }

    const auto type = CandleType::parse(typeStr);

    return valueOf(*value, type.get());
}

CandlePeriod CandlePeriod::valueOf(double value, const CandleType &type) {
    if (math::exactlyEquals(value, 1.0) && type == CandleType::DAY) {
        return DAY;
    }

    if (math::exactlyEquals(value, 1.0) && type == CandleType::TICK) {
        return TICK;
    }

    return {value, type};
}

CandlePeriod CandlePeriod::getAttributeForSymbol(const StringLike &symbol) {
    const auto string = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    return !string ? DEFAULT : parse(string.value());
}

std::string CandlePeriod::normalizeAttributeForSymbol(const StringLike &symbol) {
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
