// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/utils/StringUtils.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>

namespace dxfcpp {

/**
 * Candle alignment attribute of CandleSymbol defines how candle are aligned with respect to time.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via
 * CandleAlignment::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to
 * the corresponding CandleAlignment::toString()
 */
struct DXFCPP_EXPORT CandleAlignment : public CandleSymbolAttribute {
    /**
     * Align candles on midnight.
     */
    static const CandleAlignment MIDNIGHT;

    /**
     * Align candles on trading sessions.
     */
    static const CandleAlignment SESSION;

    /**
     * Default alignment is ::MIDNIGHT.
     */
    static const CandleAlignment DEFAULT;

    /**
     * The attribute key that is used to store the value of CandleAlignment in
     * a symbol string using methods of MarketEventSymbols class.
     * The value of this constant is "a".
     * The value that this key shall be set to is equal to the corresponding ::toString()
     */
    static const std::string ATTRIBUTE_KEY;

  private:
    static const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>> BY_STRING_;

    std::string string_{};

    explicit CandleAlignment(const std::string &string) noexcept : string_{string} {
    }

  public:
    CandleAlignment() noexcept = default;

    /**
     * Returns candle event symbol string with this candle alignment set.
     *
     * @param symbol The original candle event symbol.
     * @return candle event symbol string with this candle alignment set.
     */
    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Returns string representation of this candle alignment.
     * The string representation of candle alignment "m" for ::MIDNIGHT and "s" for ::SESSION.
     *
     * @return string representation of this candle alignment.
     */
    std::string toString() const noexcept {
        return string_;
    }

    bool operator==(const CandleAlignment &candleAlignment) const noexcept {
        return string_ == candleAlignment.string_;
    }

    /**
     * Parses string representation of candle alignment into object.
     * Any string that was returned by ::toString() can be parsed and case is ignored for parsing.
     *
     * @param s The string representation of candle alignment.
     * @return The candle alignment (reference) | std::nullopt if there is no supported attribute's value.
     */
    static std::optional<std::reference_wrapper<const CandleAlignment>> parse(const std::string &s) noexcept {
        auto found = BY_STRING_.find(s);

        if (found != BY_STRING_.end()) {
            return found->second;
        }

        for (const auto &a : BY_STRING_) {
            if (iEquals(a.first, s)) {
                return a.second;
            }
        }

        // TODO: error handling throw IllegalArgumentException("Unknown candle alignment: " + s);

        return std::nullopt;
    }

    /**
     * Returns candle alignment of the given candle symbol string.
     * The result is ::DEFAULT if the symbol does not have candle alignment attribute or std::nullopt if there is no
     * supported attribute's value.
     *
     * @param symbol The candle symbol string.
     * @return candle alignment of the given candle symbol string | std::nullopt if there is no supported attribute's
     * value.
     */
    static std::optional<std::reference_wrapper<const CandleAlignment>>
    getAttributeForSymbol(const std::string &symbol) noexcept {
        auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !stringOpt ? DEFAULT : parse(stringOpt.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle alignment attribute.
     *
     * @param symbol The candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle alignment attribute.
     */
    static DXFCPP_CXX20_CONSTEXPR_STRING std::string normalizeAttributeForSymbol(const std::string &symbol) noexcept {
        auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        if (!a) {
            return symbol;
        }

        auto other = parse(a.value());

        if (other.has_value()) {
            if (other.value() == DEFAULT) {
                return MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY);
            }

            if (a.value() != other.value().get().toString()) {
                return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY,
                                                                      other.value().get().toString());
            }
        }

        return symbol;
    }
};

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::CandleAlignment> {
    std::size_t operator()(const dxfcpp::CandleAlignment &candleAlignment) const noexcept {
        return std::hash<std::string>{}(candleAlignment.toString());
    }
};