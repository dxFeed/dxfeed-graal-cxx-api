// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/utils/StringUtils.hpp"
#include "../../schedule/SessionFilter.hpp"
#include "../market/MarketEventSymbols.hpp"
#include "./CandleSymbolAttribute.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * Session attribute of CandleSymbol defines trading used to build the candles.
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
struct DXFCPP_EXPORT CandleSession final : CandleSymbolAttribute {
    /**
     * All trading sessions are used to build candles.
     */
    static const CandleSession ANY;

    /**
     * Only regular trading session data is used to build candles.
     */
    static const CandleSession REGULAR;

    /**
     * The default trading session is CandleSession::ANY.
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
    const SessionFilter *sessionFilter_{};
    std::string string_;

    CandleSession(const SessionFilter &sessionFilter, const StringLike &string) noexcept;

    public:
    CandleSession() noexcept = default;
    virtual ~CandleSession() noexcept = default;

    /**
     * Returns a session filter that corresponds to this session attribute.
     * @return The session filter that corresponds to this session attribute.
     */
    const SessionFilter &getSessionFilter() const & noexcept;

    /**
     * Returns candle event symbol string with this session attribute set.
     * @param symbol original candle event symbol.
     * @return candle event symbol string with this session attribute set.
     */
    std::string changeAttributeForSymbol(const StringLike &symbol) const override;

    /**
     * Returns string representation of this candle session attribute.
     * The string representation of the candle session attribute is a lower case string that corresponds to its
     * @ref CandleSession::name() "name". For example, CandleSession::ANY is represented as "any".
     *
     * @return string representation of this candle session attribute.
     */
    const std::string &toString() const & noexcept;

    bool operator==(const CandleSession &candleSession) const noexcept;

    /**
     * Parses string representation of the candle session attribute into an object.
     * Any string returned by CandleSession::toString() can be parsed, and a case is ignored for parsing.
     *
     * @param s The string representation of the candle session attribute.
     * @return The candle session attribute (reference).
     * @throws InvalidArgumentException if the argument is empty or invalid
     */
    static std::reference_wrapper<const CandleSession> parse(const StringLike &s);

    /**
     * Returns candle session attribute of the given candle symbol string.
     * The result is CandleSession::DEFAULT if the symbol does not have a candle session attribute.
     *
     * @param symbol The candle symbol string.
     * @return The candle session attribute of the given candle symbol string.
     */
    static std::reference_wrapper<const CandleSession> getAttributeForSymbol(const StringLike &symbol);

    /**
     * Returns candle symbol string with the normalized representation of the candle session attribute.
     *
     * @param symbol candle symbol string.
     * @return The candle symbol string with the normalized representation of the candle session attribute.
     */
    static std::string normalizeAttributeForSymbol(const StringLike &symbol) noexcept;
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CandleSession> {
    std::size_t operator()(const dxfcpp::CandleSession &candleSession) const noexcept {
        return std::hash<std::string>{}(candleSession.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()