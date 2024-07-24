// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/utils/StringUtils.hpp"
#include "../../schedule/SessionFilter.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "CandleSymbolAttribute.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * Session attribute of CandleSymbol defines trading that is used to build the candles.
 *
 * <h3>Implementation details</h3>
 *
 * This attribute is encoded in a symbol string with
 * @ref MarketEventSymbols::getAttributeStringByKey() "MarketEventSymbols.getAttributeStringByKey",
 * @ref MarketEventSymbols::changeAttributeStringByKey() "changeAttributeStringByKey", and
 * @ref MarketEventSymbols::removeAttributeStringByKey() "removeAttributeStringByKey" methods.
 *
 * <p> CandleSession::ANY session is a default.
 * The key to use with these methods is available via CandleSession::ATTRIBUTE_KEY constant.
 * The value that this key shall be set to is equal to the corresponding CandleSession::toString()
 */
struct DXFCPP_EXPORT CandleSession : public CandleSymbolAttribute {
    /**
     * All trading sessions are used to build candles.
     */
    static const CandleSession ANY;

    /**
     * Only regular trading session data is used to build candles.
     */
    static const CandleSession REGULAR;

    /**
     * Default trading session is CandleSession::ANY.
     */
    static const CandleSession DEFAULT;

    /**
     * The attribute key that is used to store the value of CandleSession in
     * a symbol string using methods of MarketEventSymbols class.
     * The value of this constant is "tho", which is an abbreviation for "trading hours only".
     * The value that this key shall be set to is equal to
     * the corresponding CandleSession::toString()
     */
    static const std::string ATTRIBUTE_KEY;

    static const std::unordered_map<std::string, std::reference_wrapper<const CandleSession>> BY_STRING;
    static const std::vector<std::reference_wrapper<const CandleSession>> VALUES;

  private:
    const SessionFilter *sessionFilter_;
    std::string string_;

    CandleSession(const SessionFilter &sessionFilter, const dxfcpp::StringLikeWrapper &string) noexcept
        : sessionFilter_{&sessionFilter}, string_{string} {
    }

  public:
    CandleSession() noexcept = default;
    virtual ~CandleSession() noexcept = default;

    /**
     * Returns session filter that corresponds to this session attribute.
     * @return session filter that corresponds to this session attribute.
     */
    const SessionFilter &getSessionFilter() const noexcept {
        return *sessionFilter_;
    }

    /**
     * Returns candle event symbol string with this session attribute set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this session attribute set.
     */
    std::string changeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) const override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    /**
     * Returns string representation of this candle session attribute.
     * The string representation of candle session attribute is a lower case string that corresponds to its
     * @ref CandleSession::name() "name". For example, CandleSession::ANY is represented as "any".
     *
     * @return string representation of this candle session attribute.
     */
    const std::string &toString() const & noexcept {
        return string_;
    }

    bool operator==(const CandleSession &candleSession) const noexcept {
        return *sessionFilter_ == candleSession.getSessionFilter() && string_ == candleSession.toString();
    }

    /**
     * Parses string representation of candle session attribute into object.
     * Any string that was returned by CandleSession::toString() can be parsed and case is ignored for parsing.
     *
     * @param s The string representation of candle session attribute.
     * @return The candle session attribute (reference).
     * @throws InvalidArgumentException if argument is empty or invalid
     */
    static std::reference_wrapper<const CandleSession> parse(const dxfcpp::StringLikeWrapper &s) {
        if (s.empty()) {
            throw InvalidArgumentException("Missing candle session");
        }

        auto found = BY_STRING.find(s);

        if (found != BY_STRING.end()) {
            return found->second;
        }

        for (const auto &sessionRef : VALUES) {
            const auto &sessionStr = sessionRef.get().toString();

            if (sessionStr.length() >= s.length() && iEquals(sessionStr.substr(0, s.length()), s)) {
                return sessionRef;
            }
        }

        throw InvalidArgumentException("Unknown candle session: " + s);
    }

    /**
     * Returns candle session attribute of the given candle symbol string.
     * The result is CandleSession::DEFAULT if the symbol does not have candle session attribute.
     *
     * @param symbol The candle symbol string.
     * @return candle session attribute of the given candle symbol string.
     */
    static std::reference_wrapper<const CandleSession> getAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) {
        auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

        return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
    }

    /**
     * Returns candle symbol string with the normalized representation of the candle session attribute.
     *
     * @param symbol candle symbol string.
     * @return candle symbol string with the normalized representation of the the candle session attribute.
     */
    static std::string normalizeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) noexcept {
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

template <> struct std::hash<dxfcpp::CandleSession> {
    std::size_t operator()(const dxfcpp::CandleSession &candleSession) const noexcept {
        return std::hash<std::string>{}(candleSession.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()