// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePeriod.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleType.hpp"

DXFCPP_BEGIN_NAMESPACE

CandlePeriod::CandlePeriod(double value, const CandleType &type) noexcept : value_{value}, type_{type} {
}

CandlePeriod::CandlePeriod() noexcept {
}

CandlePeriod::~CandlePeriod() noexcept {
}

std::int64_t CandlePeriod::getPeriodIntervalMillis() const noexcept {
    return static_cast<std::int64_t>(static_cast<double>(type_.getPeriodIntervalMillis()) * value_);
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
    if (string_.empty()) {
        string_ = math::equals(value_, DEFAULT_PERIOD_VALUE) ? type_.toString()
                  : math::equals(value_, static_cast<std::int64_t>(value_))
                      ? std::to_string(static_cast<std::int64_t>(value_)) + "" + type_.toString()
                      : std::to_string(value_) + "" + type_.toString();
    }

    return string_;
}

bool CandlePeriod::operator==(const CandlePeriod &candlePeriod) const {
    return math::equals(value_, candlePeriod.getValue()) && type_ == candlePeriod.getType();
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
    // ReSharper disable once CppFunctionalStyleCast
    const auto value = valueStr.empty() ? 1.0 : double(s);
    const auto type = CandleType::parse(typeStr);

    return valueOf(value, type.get());
}

CandlePeriod CandlePeriod::valueOf(double value, const CandleType &type) noexcept {
    if (value == 1 && type == CandleType::DAY) {
        return DAY;
    }

    if (value == 1 && type == CandleType::TICK) {
        return TICK;
    }

    return {value, type};
}

CandlePeriod CandlePeriod::getAttributeForSymbol(const StringLike &symbol) noexcept {
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