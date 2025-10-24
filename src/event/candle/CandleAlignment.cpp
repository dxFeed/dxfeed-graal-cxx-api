// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/candle/CandleAlignment.hpp>

#include <dxfeed_graal_cpp_api/event/market/MarketEventSymbols.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/internal/utils/StringUtils.hpp>

DXFCPP_BEGIN_NAMESPACE

const CandleAlignment CandleAlignment::MIDNIGHT{"m"};
const CandleAlignment CandleAlignment::SESSION{"s"};
const CandleAlignment CandleAlignment::DEFAULT = MIDNIGHT;
const std::string CandleAlignment::ATTRIBUTE_KEY{"a"};

const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, dxfcpp::StringHash,
                         std::equal_to<>>
    CandleAlignment::BY_STRING{
        {CandleAlignment::MIDNIGHT.toString(), std::cref(CandleAlignment::MIDNIGHT)},
        {CandleAlignment::SESSION.toString(), std::cref(CandleAlignment::SESSION)},
    };

const std::vector<std::reference_wrapper<const CandleAlignment>> CandleAlignment::VALUES{
    std::cref(CandleAlignment::MIDNIGHT),
    std::cref(CandleAlignment::SESSION),
};

CandleAlignment::CandleAlignment(const dxfcpp::StringLikeWrapper &string) noexcept : string_{string} {
}

std::string CandleAlignment::changeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
}

std::string CandleAlignment::toString() const {
    return string_;
}

bool CandleAlignment::operator==(const CandleAlignment &candleAlignment) const noexcept {
    return string_ == candleAlignment.string_;
}

std::reference_wrapper<const CandleAlignment> CandleAlignment::parse(const dxfcpp::StringLikeWrapper &s) {
    if (auto found = BY_STRING.find(s); found != BY_STRING.end()) {
        return found->second;
    }

    for (const auto &alignmentRef : VALUES) {
        const auto &alignmentStr = alignmentRef.get().toString();

        if (iEquals(alignmentStr, s)) {
            return alignmentRef;
        }
    }

    throw InvalidArgumentException("Unknown candle alignment: " + s);
}

std::reference_wrapper<const CandleAlignment>
CandleAlignment::getAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
    auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandleAlignment::normalizeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
    auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    if (!a) {
        return symbol;
    }

    try {
        auto other = parse(a.value());

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