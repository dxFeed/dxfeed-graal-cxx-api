// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../detail/StringParam.hpp"
#include "./CandleAlignment.hpp"
#include "./CandleDataType.hpp"
#include "./CandleExchange.hpp"
#include "./CandlePeriod.hpp"
#include "./CandlePrice.hpp"
#include "./CandlePriceLevel.hpp"
#include "./CandleSession.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

/**
 * @page candle_symbols Candle symbols
 *
 * A Candle event is requested with a candle symbol: a market symbol with attributes that say how to build the
 * candles. The symbol is a CandleSymbol object or its string, `AAPL{=5m,price=bid}`: the base symbol, an optional
 * `&` with an exchange code letter and an optional list of `key=value` pairs in curly braces. CandleSymbol::valueOf()
 * and the `_c` literal parse such a string, the overloads of CandleSymbol::valueOf() with attributes build a symbol
 * from objects: `CandleSymbol::valueOf("AAPL", CandlePeriod::valueOf(5, CandleType::MINUTE))`.
 *
 * <h3>Period</h3>
 *
 * The period has an empty key, `{=5m}`: an optional value (1 by default, a fraction and the exponential form are
 * allowed: `{=0.5h}`, `{=1.5E1m}`) and a type. A symbol without a period gives tick candles (CandlePeriod::TICK).
 *
 * | Type                         | String | Example        |
 * |------------------------------|--------|----------------|
 * | CandleType::TICK             | `t`    | `AAPL{=100t}`  |
 * | CandleType::SECOND           | `s`    | `AAPL{=30s}`   |
 * | CandleType::MINUTE           | `m`    | `AAPL{=5m}`    |
 * | CandleType::HOUR             | `h`    | `AAPL{=h}`     |
 * | CandleType::DAY              | `d`    | `AAPL{=d}`     |
 * | CandleType::WEEK             | `w`    | `AAPL{=w}`     |
 * | CandleType::MONTH            | `mo`   | `AAPL{=mo}`    |
 * | CandleType::OPTEXP           | `o`    | `AAPL{=o}`     |
 * | CandleType::YEAR             | `y`    | `AAPL{=y}`     |
 * | CandleType::VOLUME           | `v`    | `AAPL{=1000v}` |
 * | CandleType::PRICE            | `p`    | `AAPL{=0.5p}`  |
 * | CandleType::PRICE_MOMENTUM   | `pm`   | `AAPL{=0.5pm}` |
 * | CandleType::PRICE_RENKO      | `pr`   | `AAPL{=0.5pr}` |
 *
 * The type strings of the table are the canonical form, CandleSymbol::toString() writes them: a price is `p`. Like
 * Java, the parser also takes the name of a type or the start of the name in any case (`{=5min}`, `{=day}`). An exact
 * type string is taken first, and the start of a name is matched against the types in the order of the table, so a
 * short string in a wrong case gives another type: `M` is a minute (a month is `mo`), `PR` and `Pr` are a price (a
 * Renko price is `pr`). Both are typical mistakes, see below.
 *
 * <h3>Other attributes</h3>
 *
 * | Attribute                           | Key     | Values                                                     |
 * |-------------------------------------|---------|------------------------------------------------------------|
 * | @ref CandlePrice "price"            | `price` | `last` (default), `bid`, `ask`, `mark`, `s` (settlement)   |
 * | @ref CandleSession "session"        | `tho`   | `false` (all sessions, default), `true` (regular hours)    |
 * | @ref CandleAlignment "alignment"    | `a`     | `m` (at midnight, default), `s` (at the session start)     |
 * | @ref CandlePriceLevel "price level" | `pl`    | a number, `NaN` by default                                 |
 * | @ref CandleDataType "data type"     | `dt`    | any string, for example `source`, `tho`, `all`, `rth`, `eth` |
 * | @ref CandleExchange "exchange"      | `&X`    | after the base symbol, `AAPL&Q{=5m}`; composite by default |
 *
 * Examples:
 *
 * - `AAPL{=d}`: daily candles.
 * - `AAPL{=15m,tho=true}`: 15-minute candles of the regular trading hours.
 * - `AAPL{=h,a=s,tho=true}`: hourly candles that start at the start of the trading session (9:30 am ET), not at 9:00.
 * - `EUR/USD{=m,price=bid}`: one-minute candles of the bid price.
 * - `/ES{=d,price=s}`: daily candles of the settlement price.
 *
 * <h3>Typical mistakes</h3>
 *
 * A wrong candle symbol is valid for the parser and for the feed, so it gives no error, only no candles or other
 * candles than expected.
 *
 * - **No period.** `AAPL` gives tick candles, not daily or minute ones. Write the period: `AAPL{=d}`, `AAPL{=1m}`, or
 *   `AAPL{=t}` when the tick candles are meant.
 * - **`M` for a month.** `AAPL{=5M}` is 5 minutes, a month is `AAPL{=mo}`.
 * - **`PR` for a Renko price.** `AAPL{=2PR}` is a price period, a Renko one is `AAPL{=2pr}`.
 * - **The price of the trades for an instrument without trades.** The default price is `last`, the price of the
 *   trades. Forex and other instruments that are quoted, not traded, need `price=bid`, `price=ask` or `price=mark`.
 * - **The session.** Without `tho=true` the candles are built from the data of all sessions (the default daily candles
 *   of US equities are special, see CandleSymbol); hourly candles of the regular trading hours that start at the
 *   start of the session need `a=s` too.
 * - **A symbol passed as a string.** The string of a tick candle symbol has no period:
 *   `CandleSymbol::valueOf("AAPL{=t}").toString()` is `AAPL`. Pass the CandleSymbol object, not its string, where a
 *   symbol is expected.
 *
 * How far back the history of the candles goes depends on the period and on the data source: the shorter the period,
 * the shorter the history.
 *
 * <h3>Warnings</h3>
 *
 * CandleSymbol::valueOf() of a string and the `_c` literal report a warning for the first three mistakes through
 * ErrorManager (see @ref error_handling "Error handling"), and the symbol is created as before:
 *
 * `[dxfcpp] WARN #1 CandleSymbol (CandleSymbol.cpp:NNN): Candle symbol 'AAPL' has no period and gives tick candles; for
 * other candles add a period, for example 'AAPL{=1m}' or 'AAPL{=d}', for tick candles write 'AAPL{=t}'`
 *
 * A warning is reported once per string, for the first 1000 strings in the process. The symbols of the received
 * candles, the overloads of CandleSymbol::valueOf() with attributes and a string given to a subscription directly are
 * not checked.
 */

/**
 * \addtogroup dxfcpp_candle
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The common "variant" type for the candle attribute types
 */
using CandleSymbolAttributeVariant = std::variant<CandleExchange, CandlePrice, CandleSession, CandlePeriod,
                                                  CandleAlignment, CandlePriceLevel, CandleDataType>;

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
 * <p><code>&lt;baseSymbol&gt; [ '&amp;' &lt;exchange&gt; ]
 * [ '{' &lt;key1&gt;=&lt;value1&gt; [ ',' &lt;key2&gt;=&lt;value2&gt; ] ... '}' ]</code>
 *
 * <p>Properties of the candle symbol correspond to the keys in the string representation in the following way:
 *
 * <ul>
 * <li>Empty key corresponds to @ref CandleSymbol::getPeriod() "period" &mdash; aggregation period of this symbol.
 *     The period value is composed of an optional @ref CandlePeriod::getValue() "value" which defaults to 1 when not
 *     specified, followed by a @ref CandlePeriod::getType() "type" string which is defined by one of the CandleType
 *     values and can be abbreviated to first letters. For example, a daily candle of "IBM" base symbol can be
 *     specified as "IBM{=d}" and 15 minute candle on it as "IBM{=15m}". The shortest possible abbreviation for
 *     CandleType::MONTH is "mo", so the monthly candle can be specified as "IBM{=mo}". When a period is not specified,
 *     then the CandlePeriod::TICK aggregation period is assumed as default. Note that tick aggregation may not be
 *     available on the demo system, which is limited to a subset of symbols and aggregation periods.
 * <li>"price" key corresponds to @ref CandleSymbol::getPrice() "price" &mdash; price type attribute of this symbol.
 *     The CandlePrice defines possible values with CandlePrice::LAST being default.
 *     For legacy backwards-compatibility purposes, most of the price values cannot be abbreviated, so a one-minute
 *     candle of "EUR/USD" bid price shall be specified with "EUR/USD{=m,price=bid}" candle symbol string. However, the
 *     CandlePrice::SETTLEMENT can be abbreviated to "s", so a daily candle on "/ES" futures settlement prices can be
 *     specified with "/ES{=d,price=s}" string.
 * <li>"tho" key with a value of "true" corresponds to @ref CandleSymbol::getSession() "session" set to
 * CandleSession::REGULAR which limits the candle to trading hours only, so a 133 tick candles on "GOOG" base symbol
 * collected over trading hours only can be specified with "GOOG{=133t,tho=true}" string. Note that the default daily
 * candles for US equities are special for historical reasons and correspond to the way US equity exchange report their
 *     daily summary data. The volume of the US equity default daily candle corresponds to the total daily traded
 *     volume, while open, high, low, and close correspond to the regular trading hours only.
 * <li>"a" key corresponds to @ref CandleSymbol::getAlignment() "alignment" &mdash; alignment attribute of this symbol.
 *     The CandleAlignment defines possible values with CandleAlignment::MIDNIGHT being default. The alignment values
 *     can be abbreviated to the first letter. So, a 1 hour candle on a symbol "AAPL" that starts at the regular
 *     trading session at 9:30 am ET can be specified with "AAPL{=h,a=s,tho=true}". Contrast that to
 *     the "AAPL{=h,tho=true}" candle that is aligned at midnight and thus starts at 9:00 am.
 * <li>"pl" key corresponds to @ref CandleSymbol::getPriceLevel() "price level" &mdash; price level attribute of this
 * symbol. The CandlePriceLevel defines additional axis to split candles within particular price corridor in addition to
 *     CandlePeriod attribute with the default value `NaN`. So a one-minute candles of "AAPL" with price level 0.1
 *     shall be specified with "AAPL{=m,pl=0.1}".
 * <li>"dt" key corresponds to @ref CandleSymbol::getDataType() "data type" &mdash; data type attribute of this symbol.
 *     The CandleDataType defines which types of data are used to build the candles, with CandleDataType::UNDEFINED
 *     being default. Any string is a valid data type, and its value is case-sensitive, so one-minute candles of
 *     "AAPL" built from the extended trading hours data shall be specified with "AAPL{=m,dt=eth}".
 * </ul>
 *
 * Keys in the candle symbol are case-sensitive, while values (except the data type) are not. The
 * CandleSymbol::valueOf() method parses any valid string representation into a candle symbol object. The result of the
 * candle symbol CandleSymbol::toString() method is always normalized: keys are ordered lexicographically, values (except
 * the data type) are in lower-case and are abbreviated to their shortest possible form.
 *
 * The page @ref candle_symbols "Candle symbols" lists the attributes, examples and the typical mistakes.
 */
struct DXFCPP_EXPORT CandleSymbol {
    /// Returns the candle symbol "<null>".
    static CandleSymbol NUL();

    private:
    std::string symbol_{};
    std::string baseSymbol_{};
    std::optional<CandleExchange> exchange_{};
    std::optional<CandlePrice> price_{};
    std::optional<CandleSession> session_{};
    std::optional<CandlePeriod> period_{};
    std::optional<CandleAlignment> alignment_{};
    std::optional<CandlePriceLevel> priceLevel_{};
    std::optional<CandleDataType> dataType_{};

    static std::string changeAttribute(StringParam symbol, const CandleSymbolAttributeVariant &attribute);

    template <typename AttributeIt>
    static std::string changeAttributes(StringParam symbol, AttributeIt begin, AttributeIt end) {
        auto result = std::string(symbol.view());

        for (auto it = begin; it != end; ++it) {
            result = changeAttribute(result, *it);
        }

        return result;
    }

    static std::string normalize(StringParam symbol);

    void initTransientFields(bool force = false);

    explicit CandleSymbol(StringParam symbol);

    CandleSymbol(StringParam symbol, const CandleSymbolAttributeVariant &attribute);

    template <typename CandleSymbolAttributeIt>
    CandleSymbol(StringParam symbol, CandleSymbolAttributeIt begin, CandleSymbolAttributeIt end)
        : symbol_{normalize(changeAttributes(symbol, begin, end))} {
        initTransientFields();
    }

    public:
    /**
     * Returns base market symbol without attributes.
     *
     * @return base market symbol without attributes.
     */
    const std::string &getBaseSymbol() const & noexcept;

    /**
     * Returns exchange attribute of this symbol.
     *
     * @return exchange attribute of this symbol.
     */
    const std::optional<CandleExchange> &getExchange() const & noexcept;

    /**
     * Returns the price type attribute of this symbol.
     *
     * @return price type attribute of this symbol.
     */
    const std::optional<CandlePrice> &getPrice() const & noexcept;

    /**
     * Returns the session attribute of this symbol.
     *
     * @return session attribute of this symbol.
     */
    const std::optional<CandleSession> &getSession() const & noexcept;

    /**
     * Returns the aggregation period of this symbol.
     *
     * @return aggregation period of this symbol.
     */
    const std::optional<CandlePeriod> &getPeriod() const & noexcept;

    /**
     * Returns alignment attribute of this symbol.
     *
     * @return alignment attribute of this symbol.
     */
    const std::optional<CandleAlignment> &getAlignment() const & noexcept;

    /**
     * Returns the price level attribute of this symbol.
     *
     * @return price level attribute of this symbol.
     */
    const std::optional<CandlePriceLevel> &getPriceLevel() const & noexcept;

    /**
     * Returns the data type attribute of this symbol.
     *
     * @return data type attribute of this symbol.
     */
    const std::optional<CandleDataType> &getDataType() const & noexcept;

    /**
     * Returns string representation of this symbol.
     * The string representation can be transformed back into a symbol object
     * using the CandleSymbol::valueOf() method.
     *
     * @return string representation of this symbol.
     */
    const std::string &toString() const & noexcept;

    bool operator==(const CandleSymbol &candleSymbol) const noexcept;

    bool operator<(const CandleSymbol &candleSymbol) const noexcept;

    CandleSymbol(const CandleSymbol &candleSymbol);
    CandleSymbol(CandleSymbol &&candleSymbol);
    CandleSymbol &operator=(const CandleSymbol &candleSymbol);
    CandleSymbol &operator=(CandleSymbol &&candleSymbol);
    CandleSymbol() noexcept;
    virtual ~CandleSymbol() noexcept;

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
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
     * @throws InvalidArgumentException
     */
    static CandleSymbol fromGraal(void *graalNative);

    /**
     * Converts the given string symbol into the candle symbol object.
     *
     * A string without a period (it gives tick candles) or with a period of the type `M` or `PR` gets a warning through
     * ErrorManager, once per string, see @ref candle_symbols "Candle symbols". The symbol is created anyway.
     *
     * @param symbol The string symbol.
     * @return The candle symbol object.
     * @throws InvalidArgumentException if the string cannot be parsed.
     */
    static CandleSymbol valueOf(StringParam symbol);

    /**
     * Converts the given string symbol into the candle symbol object with the specified attribute set.
     *
     * @param symbol The string symbol.
     * @param attribute The attribute to set.
     * @return The candle symbol object.
     */
    static CandleSymbol valueOf(StringParam symbol, const CandleSymbolAttributeVariant &attribute);

    /**
     * Converts the given string symbol into the candle symbol object with the specified attribute set (iterators).
     *
     * @tparam CandleSymbolAttributeIt The attribute iterator type.
     * @param symbol The string symbol.
     * @param begin The beginning of the collection of attributes.
     * @param end The end of the collection of attributes.
     * @return
     */
    template <typename CandleSymbolAttributeIt>
    static CandleSymbol valueOf(StringParam symbol, CandleSymbolAttributeIt begin, CandleSymbolAttributeIt end) {
        return CandleSymbol{symbol, begin, end};
    }

    /**
     * Converts the given string symbol into the candle symbol object with the specified attributes set (initializer
     * list).
     *
     * @param symbol The string symbol.
     * @param attributes More attributes to set.
     * @return The candle symbol object.
     */
    static CandleSymbol valueOf(StringParam symbol, std::initializer_list<CandleSymbolAttributeVariant> attributes);

    /**
     * Converts the given string symbol into the candle symbol object with the specified attributes set.
     *
     * @tparam CandleSymbolAttributesCollection The collection of attributes type.
     * @param symbol The string symbol.
     * @param attributes More attributes to set.
     * @return The candle symbol object.
     */
    template <typename CandleSymbolAttributesCollection>
        requires requires(CandleSymbolAttributesCollection attributes) {
            { std::begin(attributes) };
            { std::end(attributes) };
        }
    static CandleSymbol valueOf(StringParam symbol, CandleSymbolAttributesCollection &&attributes) {
        return valueOf(symbol, std::begin(attributes), std::end(attributes));
    }
};

inline namespace literals {

/**
 * String literal that helps to construct CandleSymbol from a char array. It checks the string as
 * CandleSymbol::valueOf() does.
 *
 * @param string The char array
 * @param length Tha char array's length
 * @return Wrapped string view built on char array
 */
DXFCPP_EXPORT CandleSymbol operator""_c(const char *string, size_t length);

} // namespace literals

DXFCPP_END_NAMESPACE

/// @}

template <> struct std::hash<dxfcpp::CandleSymbol> {
    std::size_t operator()(const dxfcpp::CandleSymbol &candleSymbol) const noexcept {
        return std::hash<std::string>{}(candleSymbol.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
