// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp"
#include "../../internal/resources/Strings.hpp"

#include <fmt/format.h>

#include "../../internal/utils/CandleUtils.hpp"
#include "../../internal/Common.hpp"

DXFCPP_BEGIN_NAMESPACE

CandlePrice::CandlePrice() noexcept {
}

std::string CandlePrice::changeAttributeForSymbol(StringParam symbol) const {
    return *this == DEFAULT
               ? MarketEventSymbols::removeAttributeStringByKey(symbol, CandlePrice::ATTRIBUTE_KEY)
               : MarketEventSymbols::changeAttributeStringByKey(symbol, CandlePrice::ATTRIBUTE_KEY, toString());
}

std::string CandlePrice::toString() const {
    return std::string{string_};
}

bool CandlePrice::operator==(const CandlePrice &candlePrice) const noexcept {
    return string_ == candlePrice.string_;
}

std::reference_wrapper<const CandlePrice> CandlePrice::parse(StringParam s) {
    if (s.empty()) {
        throw InvalidArgumentException(fmt::format(ires::Strings::Events::MISSING_CANDLE_, "price"));
    }

    for (const auto &priceRef : VALUES) {
        if (priceRef.get().string_ == s.view()) {
            return priceRef;
        }
    }

    for (const auto &priceRef : VALUES) {
        if (const auto priceStr = priceRef.get().toString();
            priceStr.length() >= s.size() && iEquals(priceStr.substr(0, s.size()), s.view())) {
            return priceRef;
        }
    }

    throw InvalidArgumentException(fmt::format(ires::Strings::Events::UNKNOWN_CANDLE_, "price", s.view()));
}

std::reference_wrapper<const CandlePrice> CandlePrice::getAttributeForSymbol(StringParam symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, CandlePrice::ATTRIBUTE_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandlePrice::normalizeAttributeForSymbol(StringParam symbol) {
    const std::string_view sv = symbol.view();
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, CandlePrice::ATTRIBUTE_KEY);

    if (!a) {
        return std::string{sv};
    }

    try {
        const auto other = parse(a.value());

        if (other.get() == DEFAULT) {
            return candle::removeDefaultAttribute(symbol.view(), CandlePrice::ATTRIBUTE_KEY, DEFAULT.toString());
        }

        if (a.value() != other.get().toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, CandlePrice::ATTRIBUTE_KEY,
                                                                  other.get().toString());
        }

        return std::string{sv};
    } catch (const InvalidArgumentException &) {
        return std::string{sv};
    } catch (const std::invalid_argument &) {
        return std::string{sv};
    }
}

DXFCPP_END_NAMESPACE
