// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "CandleAlignment.hpp"
#include "CandleExchange.hpp"
#include "CandlePeriod.hpp"
#include "CandlePrice.hpp"
#include "CandlePriceLevel.hpp"
#include "CandleSession.hpp"
#include "CandleSymbolAttribute.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

/**
 * The common "variant" type for the candle attribute types
 */
using CandleSymbolAttributeVariant =
    std::variant<CandleExchange, CandlePrice, CandleSession, CandlePeriod, CandleAlignment, CandlePriceLevel>;

/**
 * Symbol that should be used with DXFeedSubscription class to subscribe for Candle events. `DXFeedSubscription` also
 * accepts a string representation of the candle symbol for subscription.
 *
 * <h3>String representation</h3>
 *
 * The string representation of the candle symbol consist of a @ref CandleSymbol::getBaseSymbol() "baseSymbol" followed
 * by an optional '&' with an @ref CandleSymbol::getExchange() "exchange" code letter and followed by an optional list
 * of comma-separated key=value pairs in curly braces:
 *
 * <p>`<baseSymbol> [ '&' <exchange> ] [ '{' <key1>=<value1> [ ',' <key2>=<value2> ] ... '}' ]`
 *
 * <p>Properties of the candle symbol correspond to the keys in the string representation in the following way:
 *
 * <ul>
 * <li>Empty key corresponds to @ref CandleSymbol::getPeriod() "period" &mdash; aggregation period of this symbol.
 *     The period value is composed of an optional @ref CandlePeriod::getValue() "value" which defaults to 1 when not
 *     specified, followed by a @ref CandlePeriod::getType() "type" string which is defined by one of the CandleType
 *     values and can be abbreviated to first letters. For example, a daily candle of "IBM" base symbol can be
 *     specified as "IBM{=d}" and 15 minute candle on it as "IBM{=15m}". The shortest possible abbreviation for
 *     CandleType::MONTH is "mo", so the monthly candle can be specified as "IBM{=mo}". When period is not specified,
 *     then the CandlePeriod::TICK aggregation period is assumed as default. Note, that tick aggregation may not be
 *     available on the demo system which is limited to a subset of symbols and aggregation periods.
 * <li>"price" key corresponds to @ref CandleSymbol::getPrice() "price" &mdash; price type attribute of this symbol.
 *     The CandlePrice defines possible values with CandlePrice::LAST being default.
 *     For legacy backwards-compatibility purposes, most of the price values cannot be abbreviated, so a one-minute
 *     candle of "EUR/USD" bid price shall be specified with "EUR/USD{=m,price=bid}" candle symbol string. However, the
 *     CandlePrice::SETTLEMENT can be abbreviated to "s", so a daily candle on "/ES" futures settlement prices can be
 *     specified with "/ES{=d,price=s}" string.
 * <li>"tho" key with a value of "true" corresponds to @ref CandleSymbol::getSession() "session" set to
 * CandleSession::REGULAR which limits the candle to trading hours only, so a 133 tick candles on "GOOG" base symbol
 * collected over trading hours only can be specified with "GOOG{=133t,tho=true}" string. Note, that the default daily
 * candles for US equities are special for historical reasons and correspond to the way US equity exchange report their
 *     daily summary data. The volume the US equity default daily candle corresponds to the total daily traded volume,
 *     while open, high, low, and close correspond to the regular trading hours only.
 * <li>"a" key corresponds to @ref CandleSymbol::getAlignment() "alignment" &mdash; alignment attribute of this symbol.
 *     The CandleAlignment defines possible values with CandleAlignment::MIDNIGHT being default. The alignment values
 *     can be abbreviated to the first letter. So, a 1 hour candle on a symbol "AAPL" that starts at the regular
 *     trading session at 9:30 am ET can be specified with "AAPL{=h,a=s,tho=true}". Contrast that to
 *     the "AAPL{=h,tho=true}" candle that is aligned at midnight and thus starts at 9:00 am.
 * <li>"pl" key corresponds to @ref CandleSymbol::getPriceLevel() "price level" &mdash; price level attribute of this
 * symbol. The CandlePriceLevel defines additional axis to split candles within particular price corridor in addition to
 *     CandlePeriod attribute with the default value `NaN`. So a one-minute candles of "AAPL" with price level 0.1
 *     shall be specified with "AAPL{=m,pl=0.1}".
 * </ul>
 *
 * Keys in the candle symbol are case-sensitive, while values are not. The CandleSymbol::valueOf() method parses any
 * valid string representation into a candle symbol object. The result of the candle symbol CandleSymbol::toString()
 * method is always normalized: keys are ordered lexicographically, values are in lower-case and are abbreviated to
 * their shortest possible form.
 */
struct DXFCPP_EXPORT CandleSymbol {
    static const CandleSymbol NUL;

  private:
    std::string symbol_{};
    std::string baseSymbol_{};
    std::optional<CandleExchange> exchange_{};
    std::optional<CandlePrice> price_{};
    std::optional<CandleSession> session_{};
    std::optional<CandlePeriod> period_{};
    std::optional<CandleAlignment> alignment_{};
    std::optional<CandlePriceLevel> priceLevel_{};

    static std::string changeAttribute(std::string symbol, const CandleSymbolAttributeVariant &attribute) noexcept {
        return std::visit(
            [symbol = std::move(symbol)](auto &&a) {
                return a.changeAttributeForSymbol(symbol);
            },
            attribute);
    }

    static std::string changeAttributes(std::string symbol,
                                        std::initializer_list<CandleSymbolAttributeVariant> attributes) noexcept {
        for (const auto &a : attributes) {
            symbol = changeAttribute(symbol, a);
        }

        return symbol;
    }

    static std::string normalize(std::string symbol) noexcept {
        symbol = CandlePrice::normalizeAttributeForSymbol(symbol);
        symbol = CandleSession::normalizeAttributeForSymbol(symbol);
        symbol = CandlePeriod::normalizeAttributeForSymbol(symbol);
        symbol = CandleAlignment::normalizeAttributeForSymbol(symbol);
        symbol = CandlePriceLevel::normalizeAttributeForSymbol(symbol);

        return symbol;
    }

    void initTransientFields(bool force = false) noexcept {
        baseSymbol_ = MarketEventSymbols::getBaseSymbol(symbol_);

        if (!exchange_ || force) {
            exchange_.emplace(CandleExchange::getAttributeForSymbol(symbol_));
        }

        if (!price_ || force) {
            price_ = CandlePrice::getAttributeForSymbol(symbol_);
        }

        if (!session_ || force) {
            session_ = CandleSession::getAttributeForSymbol(symbol_);
        }

        if (!period_ || force) {
            period_ = CandlePeriod::getAttributeForSymbol(symbol_);
        }

        if (!alignment_ || force) {
            alignment_ = CandleAlignment::getAttributeForSymbol(symbol_);
        }

        if (!priceLevel_ || force) {
            priceLevel_ = CandlePriceLevel::getAttributeForSymbol(symbol_);
        }
    }

    explicit CandleSymbol(std::string symbol) noexcept : symbol_{normalize(std::move(symbol))} {
        initTransientFields();
    }

    CandleSymbol(std::string symbol, const CandleSymbolAttributeVariant &attribute) noexcept
        : symbol_{normalize(changeAttribute(std::move(symbol), attribute))} {
        // TODO: check attributes
        initTransientFields();
    }

    CandleSymbol(std::string symbol, std::initializer_list<CandleSymbolAttributeVariant> attributes) noexcept
        : symbol_{normalize(changeAttributes(std::move(symbol), attributes))} {
        // TODO: check attributes
        initTransientFields();
    }

  public:
    /**
     * Returns base market symbol without attributes.
     *
     * @return base market symbol without attributes.
     */
    const std::string &getBaseSymbol() const & noexcept {
        return baseSymbol_;
    }

    /**
     * Returns exchange attribute of this symbol.
     *
     * @return exchange attribute of this symbol.
     */
    const std::optional<CandleExchange> &getExchange() const & noexcept {
        return exchange_;
    }

    /**
     * Returns price type attribute of this symbol.
     *
     * @return price type attribute of this symbol.
     */
    const std::optional<CandlePrice> &getPrice() const & noexcept {
        return price_;
    }

    /**
     * Returns session attribute of this symbol.
     *
     * @return session attribute of this symbol.
     */
    const std::optional<CandleSession> &getSession() const & noexcept {
        return session_;
    }

    /**
     * Returns aggregation period of this symbol.
     *
     * @return aggregation period of this symbol.
     */
    const std::optional<CandlePeriod> &getPeriod() const & noexcept {
        return period_;
    }

    /**
     * Returns alignment attribute of this symbol.
     *
     * @return alignment attribute of this symbol.
     */
    const std::optional<CandleAlignment> &getAlignment() const & noexcept {
        return alignment_;
    }

    /**
     * Returns price level attribute of this symbol.
     *
     * @return price level attribute of this symbol.
     */
    const std::optional<CandlePriceLevel> &getPriceLevel() const & noexcept {
        return priceLevel_;
    }

    /**
     * Returns string representation of this symbol.
     * The string representation can be transformed back into symbol object
     * using CandleSymbol::valueOf() method.
     *
     * @return string representation of this symbol.
     */
    const std::string &toString() const & noexcept {
        return symbol_;
    }

    bool operator==(const CandleSymbol &candleSymbol) const noexcept {
        return symbol_ == candleSymbol.symbol_;
    }

    bool operator<(const CandleSymbol &candleSymbol) const noexcept {
        return symbol_ < candleSymbol.symbol_;
    }

    CandleSymbol(const CandleSymbol &candleSymbol) noexcept;
    CandleSymbol(CandleSymbol &&candleSymbol) noexcept;
    CandleSymbol &operator=(const CandleSymbol &candleSymbol) noexcept;
    CandleSymbol &operator=(CandleSymbol &&candleSymbol) noexcept;
    CandleSymbol() noexcept = default;

    virtual ~CandleSymbol() = default;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    virtual void *toGraal() const;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static CandleSymbol fromGraal(void *graalNative);

    /**
     * Converts the given string symbol into the candle symbol object.
     *
     * @param symbol the string symbol.
     * @return the candle symbol object.
     */
    static CandleSymbol valueOf(std::string symbol) noexcept {
        return CandleSymbol{std::move(symbol)};
    }

    /**
     * Converts the given string symbol into the candle symbol object with the specified attribute set.
     *
     * @param symbol the string symbol.
     * @param attribute the attribute to set.
     * @return the candle symbol object.
     */
    static CandleSymbol valueOf(std::string symbol, const CandleSymbolAttributeVariant &attribute) noexcept {
        return {std::move(symbol), attribute};
    }

    /**
     * Converts the given string symbol into the candle symbol object with the specified attributes set.
     *
     * @param symbol the string symbol.
     * @param attributes more attributes to set.
     * @return the candle symbol object.
     */
    static CandleSymbol valueOf(std::string symbol,
                                std::initializer_list<CandleSymbolAttributeVariant> attributes) noexcept {
        return {std::move(symbol), attributes};
    }
};

inline namespace literals {

/**
 * String literal that helps to construct CandleSymbol from a char array.
 *
 * @param string The char array
 * @param length Tha char array's length
 * @return Wrapped string view built on char array
 */
inline CandleSymbol operator""_c(const char *string, size_t length) noexcept {
    return CandleSymbol::valueOf(std::string{string, length});
}

} // namespace literals

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CandleSymbol> {
    std::size_t operator()(const dxfcpp::CandleSymbol &candleSymbol) const noexcept {
        return std::hash<std::string>{}(candleSymbol.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()