// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/resources/Strings.hpp"

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

const CandlePrice CandlePrice::LAST{"last"};
const CandlePrice CandlePrice::BID{"bid"};
const CandlePrice CandlePrice::ASK{"ask"};
const CandlePrice CandlePrice::MARK{"mark"};
const CandlePrice CandlePrice::SETTLEMENT{"s"};
const CandlePrice CandlePrice::DEFAULT = LAST;

const std::string CandlePrice::ATTRIBUTE_KEY{"price"};

const std::unordered_map<std::string, std::reference_wrapper<const CandlePrice>, StringHash, std::equal_to<>>
    CandlePrice::BY_STRING{
        {LAST.toString(), std::cref(LAST)},
        {BID.toString(), std::cref(BID)},
        {ASK.toString(), std::cref(ASK)},
        {MARK.toString(), std::cref(MARK)},
        {SETTLEMENT.toString(), std::cref(SETTLEMENT)},
    };

const std::vector<std::reference_wrapper<const CandlePrice>> CandlePrice::VALUES{
    std::cref(LAST), std::cref(BID), std::cref(ASK), std::cref(MARK), std::cref(SETTLEMENT),
};

CandlePrice::CandlePrice(const StringLike &string) noexcept : string_{string} {
}

std::string CandlePrice::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
}

const std::string &CandlePrice::toString() const & noexcept {
    return string_;
}

bool CandlePrice::operator==(const CandlePrice &candlePrice) const noexcept {
    return string_ == candlePrice.string_;
}

std::reference_wrapper<const CandlePrice> CandlePrice::parse(const StringLike &s) {
    if (s.empty()) {
        throw InvalidArgumentException(fmt::format(ires::Strings::Events::MISSING_CANDLE_, "price"));
    }

    if (const auto found = BY_STRING.find(s); found != BY_STRING.end()) {
        return found->second;
    }

    for (const auto &priceRef : VALUES) {
        if (const auto &priceStr = priceRef.get().toString();
            priceStr.length() >= s.length() && iEquals(priceStr.substr(0, s.length()), s)) {
            return priceRef;
        }
    }

    throw InvalidArgumentException(fmt::format(ires::Strings::Events::UNKNOWN_CANDLE_, "price", s.toStringView()));
}

std::reference_wrapper<const CandlePrice> CandlePrice::getAttributeForSymbol(const StringLike &symbol) noexcept {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandlePrice::normalizeAttributeForSymbol(const StringLike &symbol) {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    if (!a) {
        return symbol;
    }

    try {
        const auto other = parse(a.value());

        if (other.get() == DEFAULT) {
            return MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY);
        }

        if (a.value() != other.get().toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, other.get().toString());
        }

        return symbol;
    } catch (const InvalidArgumentException &) {
        return symbol;
    } catch (const std::invalid_argument &) {
        return symbol;
    }
}
DXFCPP_END_NAMESPACE