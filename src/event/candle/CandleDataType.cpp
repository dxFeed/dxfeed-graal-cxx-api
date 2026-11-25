// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleDataType.hpp"

#include "../../internal/Common.hpp"

#include "../../internal/utils/CandleUtils.hpp"

#include <array>
#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

namespace {
constinit const std::array CANONICAL_DATA_TYPES{
    std::cref(CandleDataType::SOURCE), std::cref(CandleDataType::THO), std::cref(CandleDataType::ALL),
    std::cref(CandleDataType::RTH),    std::cref(CandleDataType::ETH), std::cref(CandleDataType::UNDEFINED),
};
// A function-local static: valueOf() is called from the global initializers of other translation units.
struct Values {
    std::mutex mutex{};
    // The nodes are not moved by a rehash, so the views of the values stay valid.
    std::unordered_set<std::string> values{};
};

std::string_view keepValue(std::string_view value) {
    // Destroyed at exit: a data type that is not a constant must not be used by the destructors of global variables
    // that were constructed before it.
    static Values instance{};
    std::lock_guard lock(instance.mutex);

    return *instance.values.emplace(value).first;
}

} // namespace

CandleDataType::CandleDataType() noexcept = default;

std::string CandleDataType::getValue() const {
    return std::string{value_};
}

std::string CandleDataType::toString() const {
    return std::string{value_};
}

bool CandleDataType::operator==(const CandleDataType &candleDataType) const noexcept {
    return value_ == candleDataType.value_;
}

std::string CandleDataType::changeAttributeForSymbol(StringParam symbol) const {
    return *this == DEFAULT
               ? MarketEventSymbols::removeAttributeStringByKey(symbol, CandleDataType::ATTRIBUTE_KEY)
               : MarketEventSymbols::changeAttributeStringByKey(symbol, CandleDataType::ATTRIBUTE_KEY, toString());
}

CandleDataType CandleDataType::parse(StringParam s) {
    return valueOf(s);
}

CandleDataType CandleDataType::valueOf(StringParam value) {
    const std::string_view valueView{value.view()};

    if (valueView.empty()) {
        return DEFAULT;
    }

    for (const auto &type : CANONICAL_DATA_TYPES) {
        if (type.get().value_ == valueView) {
            return type;
        }
    }

    return CandleDataType{keepValue(valueView)};
}

CandleDataType CandleDataType::getAttributeForSymbol(StringParam symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, CandleDataType::ATTRIBUTE_KEY);

    return !stringOpt ? DEFAULT : parse(stringOpt.value());
}

std::string CandleDataType::normalizeAttributeForSymbol(StringParam symbol) {
    const std::string_view sv = symbol.view();
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, CandleDataType::ATTRIBUTE_KEY);

    if (!a) {
        return std::string{sv};
    }

    // Any other value is kept as is: parsing does not change it.
    return parse(a.value()) == DEFAULT
               ? candle::removeDefaultAttribute(symbol.view(), CandleDataType::ATTRIBUTE_KEY, DEFAULT.toString())
               : std::string{sv};
}

DXFCPP_END_NAMESPACE
