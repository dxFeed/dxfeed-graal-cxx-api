// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "TradeBase.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * TradeETH event is a snapshot of the price and size of the last trade during
 * extended trading hours and the extended trading hours day volume and day turnover.
 * This event is defined only for symbols (typically stocks and ETFs) with a designated
 * <b>extended trading hours</b>  (ETH, pre market and post market trading sessions).
 * It represents the most recent information that is available about
 * ETH last trade on the market at any given moment of time.
 *
 * <h3>Trading sessions</h3>
 *
 * The TradeETH event defines last trade @ref TradeETH::getPrice() "price" as officially defined
 * by the corresponding exchange for its <b>extended trading hours</b> (ETH).
 * It also includes @ref TradeETH::getDayVolume() "dayVolume" and @ref ::getDayTurnover() "dayTurnover"
 * <b>for the extended trading hours only</b> of the trading day identified by @ref TradeETH::getDayId "dayId".
 * This event is not defined for symbols that has no concept of ETH.
 *
 * <p>When the first trade of <b>regular trading hours</b> (RTH) happens, then TradeETH event is generated
 * with @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" property set to `false`. Afterwards, during RTH,
 * TradeETH event is not updated and retains information about the last trade, volume and turnover of the pre market
 * trading session.
 *
 * <p>When the first trade of <b>extended trading hours</b> (ETH) happens, then TradeETH event is generated
 * with @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" property set to `true`. Afterwards, during ETH,
 * TradeETH event is updated on each trade with the last trade information from post market trading session
 * and total volume and turnover of the pre and post market trading session (excluding the volume and turnover of a
 * regular trading session).
 *
 * Note, that during pre- and post-market sessions, Trade event also updates, but only its
 * @ref Trade::getDayVolume() "dayVolume" and @ref Trade::getDayTurnover() "dayTurnover" properties change
 * to reflect the overall official volume and turnover as reported by exchanges.
 * During post market trading session, exchanges may correct their official RTH last trading price, which results
 * in the update to Trade event.
 *
 * <h3>Volume and Turnover</h3>
 *
 * <p>Note that one can compute volume-weighted average price (VWAP) for extended trading hours by this formula:
 * <br><code>vwap = @ref TradeETH::getDayTurnover() "dayTurnover" / @ref ::getDayVolume() "dayVolume";</code>
 *
 * <h3>Daily reset</h3>
 *
 * Daily reset procedure that happens on a schedule during non-trading hours resets TradeETH
 * @ref TradeETH::getDayVolume() "dayVolume" and @ref TradeETH::getDayTurnover() "dayTurnover" to math::NaN
 * and sets @ref TradeETH::getDayId() "dayId" to the next trading day in preparation to the next day's pre-market
 * trading session (or for regular trading if there is no pre-market) while leaving all other properties intact. They
 * reflect information about the last known ETH trade until the next ETH trade happens.
 *
 * <h3>The most recent last trade price</h3>
 *
 * The most recent last trade price ("extended last price") in the market can be found by combining information from
 * both Trade and TradeETH events using @ref TradeETH::isExtendedTradingHours() "isExtendedTradingHours" method to
 * figure out which trading session had the most recent trade. The following piece of code finds the most recent last
 * trade price from the given @ref DXFeed "feed" for a given `symbol`, assuming there is a @ref DXFeedSubscription
 * "subscription" for both Trade and TradeETH events for the given `symbol`:
 *
 * <pre><tt>
 *     auto @ref Trade "trade" = feed->@ref DXFeed::getLastEvent() "getLastEvent"(Trade::create(symbol));
 *     auto @ref TradeETH "tradeEth" = feed->@ref DXFeed::getLastEvent() "getLastEvent"(TradeETH::create(symbol));
 *     <b>double</b> extLast = tradeEth->@ref TradeETH::isExtendedTradingHours() "isExtendedTradingHours"() ?
 * tradeEth->@ref TradeETH::getPrice() "getPrice"() : trade->@ref Trade::getPrice() "getPrice"();
 * </tt></pre>
 *
 * Note, that the above code works correctly for symbols that has no concept of ETH, too, because in this
 * case the DXFeed::getLastEvent() leaves default values in TradeETH
 * event properties, which means that @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" flag is `false`
 * and a regular Trade::getPrice() is used.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `TradeETH` and `TradeETH&X`
 * for regional exchange extended trade hours.
 * @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" property is internally represented as a last bit of
 * the "Flags" field of the record. Regional records do not explicitly store a field for @ref
 * TradeETH::getExchangeCode() "exchangeCode" property.
 */
class DXFCPP_EXPORT TradeETH final : public TradeBase {
    friend struct EventMapper;

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    public:
    /// The alias to a type of shared pointer to the TradeETH object
    using Ptr = std::shared_ptr<TradeETH>;

    /// The alias to a type of unique pointer to the TradeETH object
    using Unique = std::unique_ptr<TradeETH>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static Ptr fromGraal(void *graalNative);

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const override;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    ///
    void assign(std::shared_ptr<EventType> event) override;

    /// Creates new trade event with default values.
    TradeETH() noexcept = default;

    /**
     * Creates new trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TradeETH(std::string eventSymbol) noexcept : TradeBase(std::move(eventSymbol)) {
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()