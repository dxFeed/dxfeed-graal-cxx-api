// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleAlignment.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketEventSymbols.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/resources/Strings.hpp"
#include "../../internal/utils/StringUtils.hpp"

#include <fmt/format.h>

#include "../../internal/utils/CandleUtils.hpp"

DXFCPP_BEGIN_NAMESPACE

CandleAlignment::CandleAlignment() noexcept {
}

std::string CandleAlignment::changeAttributeForSymbol(StringParam symbol) const {
    return *this == DEFAULT
               ? MarketEventSymbols::removeAttributeStringByKey(symbol, CandleAlignment::ATTRIBUTE_KEY)
               : MarketEventSymbols::changeAttributeStringByKey(symbol, CandleAlignment::ATTRIBUTE_KEY, toString());
}

std::string CandleAlignment::toString() const {
    return std::string{string_};
}

bool CandleAlignment::operator==(const CandleAlignment &candleAlignment) const noexcept {
    return string_ == candleAlignment.string_;
}

std::reference_wrapper<const CandleAlignment> CandleAlignment::parse(StringParam s) {
    for (const auto &alignmentRef : VALUES) {
        if (alignmentRef.get().string_ == s.view()) {
            return alignmentRef;
        }
    }

    for (const auto &alignmentRef : VALUES) {
        if (const auto alignmentStr = alignmentRef.get().toString(); iEquals(alignmentStr, s.view())) {
            return alignmentRef;
        }
    }

    throw InvalidArgumentException(fmt::format(ires::Strings::Events::UNKNOWN_CANDLE_, "alignment", s.view()));
}

std::reference_wrapper<const CandleAlignment> CandleAlignment::getAttributeForSymbol(StringParam symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, CandleAlignment::ATTRIBUTE_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandleAlignment::normalizeAttributeForSymbol(StringParam symbol) {
    const std::string_view sv = symbol.view();
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, CandleAlignment::ATTRIBUTE_KEY);

    if (!a) {
        return std::string{sv};
    }

    try {
        const auto other = parse(a.value());

        if (other.get() == DEFAULT) {
            return candle::removeDefaultAttribute(symbol.view(), CandleAlignment::ATTRIBUTE_KEY, DEFAULT.toString());
        }

        if (a.value() != other.get().toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, CandleAlignment::ATTRIBUTE_KEY,
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