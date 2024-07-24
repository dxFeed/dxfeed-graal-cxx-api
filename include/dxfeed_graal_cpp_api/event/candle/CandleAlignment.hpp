// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/utils/StringUtils.hpp"
#include "../../exceptions/InvalidArgumentException.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * Candle alignment attribute of CandleSymbol defines how candle are aligned with respect to time.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 * The key to use with these methods is available via CandleAlignment::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandleAlignment::toString()
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
     * Default alignment is CandleAlignment::MIDNIGHT.
     */
    static const CandleAlignment DEFAULT;

    /**
     * The attribute key that is used to store the value of CandleAlignment in
     * a symbol string using methods of MarketEventSymbols class.
     * The value of this constant is "a".
     * The value that this key shall be set to is equal to the corresponding CandleAlignment::toString()
     */
    static const std::string ATTRIBUTE_KEY;

    static const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, dxfcpp::StringHash,
                                    std::equal_to<>>
        BY_STRING;
    static const std::vector<std::reference_wrapper<const CandleAlignment>> VALUES;

  private:
    std::string string_{};

    explicit CandleAlignment(const dxfcpp::StringLikeWrapper &string) noexcept : string_{string} {
    }

  public:
    CandleAlignment() noexcept = default;
    virtual ~CandleAlignment() noexcept = default;

    /**
     * Returns candle event symbol string with this candle alignment set.
     *
     * @param symbol The original candle event symbol.
     * @return candle event symbol string with this candle alignment set.
     */
    std::string changeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) const override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Returns string representation of this candle alignment.
     * The string representation of candle alignment "m" for CandleAlignment::MIDNIGHT and "s" for
     * CandleAlignment::SESSION.
     *
     * @return string representation of this candle alignment.
     */
    std::string toString() const {
        return string_;
    }

    bool operator==(const CandleAlignment &candleAlignment) const noexcept {
        return string_ == candleAlignment.string_;
    }

    /**
     * Parses string representation of candle alignment into object.
     * Any string that was returned by CandleAlignment::toString() can be parsed and case is ignored for parsing.
     *
     * @param s The string representation of candle alignment.
     * @return The candle alignment (reference)
     * @throws InvalidArgumentException if the string representation is invalid.
     */
    static std::reference_wrapper<const CandleAlignment> parse(const dxfcpp::StringLikeWrapper &s) {
        auto found = BY_STRING.find(s);

        if (found != BY_STRING.end()) {
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

    /**
     * Returns candle alignment of the given candle symbol string.
     * The result is CandleAlignment::DEFAULT if the symbol does not have candle alignment attribute.
     *
     * @param symbol The candle symbol string.
     * @return candle alignment of the given candle symbol string.
     */
    static std::reference_wrapper<const CandleAlignment>
    getAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
        auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle alignment attribute.
     *
     * @param symbol The candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle alignment attribute.
     */
    static std::string
    normalizeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
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
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CandleAlignment> {
    std::size_t operator()(const dxfcpp::CandleAlignment &candleAlignment) const noexcept {
        return std::hash<std::string>{}(candleAlignment.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()