// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleAlignment.hpp"
#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketEventSymbols.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/resources/Strings.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/utils/StringUtils.hpp"

#include <fmt/format.h>

#include "../../internal/utils/CandleUtils.hpp"

DXFCPP_BEGIN_NAMESPACE

CandleAlignment::CandleAlignment() noexcept {
}

CandleAlignment::~CandleAlignment() noexcept {
}

std::string CandleAlignment::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, candle::ALIGNMENT_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, candle::ALIGNMENT_KEY, toString());
}

std::string CandleAlignment::toString() const {
    return std::string{string_};
}

bool CandleAlignment::operator==(const CandleAlignment &candleAlignment) const noexcept {
    return string_ == candleAlignment.string_;
}

std::reference_wrapper<const CandleAlignment> CandleAlignment::parse(const StringLike &s) {
    for (const auto &alignmentRef : VALUES) {
        if (alignmentRef.get().string_ == std::string_view{s}) {
            return alignmentRef;
        }
    }

    for (const auto &alignmentRef : VALUES) {
        if (const auto alignmentStr = alignmentRef.get().toString(); iEquals(alignmentStr, s)) {
            return alignmentRef;
        }
    }

    throw InvalidArgumentException(fmt::format(ires::Strings::Events::UNKNOWN_CANDLE_, "alignment", s.toStringView()));
}

std::reference_wrapper<const CandleAlignment> CandleAlignment::getAttributeForSymbol(const StringLike &symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, candle::ALIGNMENT_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandleAlignment::normalizeAttributeForSymbol(const StringLike &symbol) {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, candle::ALIGNMENT_KEY);

    if (!a) {
        return symbol;
    }

    try {
        const auto other = parse(a.value());

        if (other.get() == DEFAULT) {
            return candle::removeDefaultAttribute(symbol, candle::ALIGNMENT_KEY, DEFAULT.toString());
        }

        if (a.value() != other.get().toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, candle::ALIGNMENT_KEY,
                                                                  other.get().toString());
        }

        return symbol;
    } catch (const InvalidArgumentException &) {
        return symbol;
    } catch (const std::invalid_argument &) {
        return symbol;
    }
}

const std::string &CandleAlignment::ATTRIBUTE_KEY() {
    static const auto *const value = new std::string{candle::ALIGNMENT_KEY};

    return *value;
}

const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, StringHash, std::equal_to<>> &
CandleAlignment::BY_STRING() {
    static const auto *const value =
        new std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, StringHash, std::equal_to<>>{
            {MIDNIGHT.toString(), std::cref(MIDNIGHT)},
            {SESSION.toString(), std::cref(SESSION)},
        };

    return *value;
}

DXFCPP_END_NAMESPACE