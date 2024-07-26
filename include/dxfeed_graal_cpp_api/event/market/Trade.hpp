// Copyright (c) 2024 Devexperts LLC.
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
 * Trade event is a snapshot of the price and size of the last trade during regular trading hours
 * and an overall day volume and day turnover.
 * It represents the most recent information that is available about the regular last trade on the market
 * at any given moment of time.
 *
 * <h3>Trading sessions</h3>
 *
 * The Trade event defines last trade @ref Trade::getPrice() "price" as officially defined
 * by the corresponding exchange for its <b>regular trading hours</b> (RTH).
 * It also include an official exchange @ref Trade::getDayVolume() "dayVolume" and @ref Trade::getDayTurnover()
 * "dayTurnover" <b>for the whole trading day</b> identified by @ref Trade::getDayId() "dayId". So, Trade event captures
 * all the official numbers that are typically reported by exchange.
 *
 * <p>Trades that happen in <b>extended trading hours</b> (ETH, pre-market and post-market trading sessions),
 * which are typically defined for stocks and ETFs, do not update last trade @ref Trade::getTime() "time",
 * @ref Trade::getExchangeCode() "exchangeCode", @ref Trade::getPrice() "price", @ref Trade::getChange() "change",
 * @ref Trade::getSize() "size", and @ref Trade::getTickDirection() "tickDirection" in the Trade event, but they do
 * update
 * @ref Trade::getDayVolume() "dayVolume" and @ref Trade::getDayTurnover() "dayTurnover".
 *
 * <p>During extended trading hours a TradeETH event is generated on each trade with its
 * @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" property set to `true`.</p>
 *
 * <h3>Volume and Turnover</h3>
 *
 * <p>The volume and turnover are included into the Trade event instead
 * of Summary event, because both volume and turnover typically update with each trade.
 * The @ref Trade::getDayId() "dayId" field identifies current trading day for which volume and turnover statistics are
 * computed. This solution avoids generation of multiple events on each trade during regular trading hours. Summary
 * event is generated during the trading day only when new highs or lows are reached or other properties change.
 *
 * <p>Note that one can compute volume-weighted average price (VWAP) for a day by this formula:
 * <br><code>vwap = @ref Trade::getDayTurnover() "dayTurnover" / @ref Trade::getDayVolume() "dayVolume";</code>
 *
 * <h3>Daily reset</h3>
 *
 * Daily reset procedure that happens on a schedule during non-trading hours resets Trade
 * @ref Trade::getDayVolume() "dayVolume" and @ref Trade::getDayTurnover() "dayTurnover" to math::NaN
 * and sets @ref Trade::getDayId() "dayId" to the next trading day in preparation to the next day's pre-market trading
 * session (or for regular trading if there is no pre-market) while leaving all other properties intact. They reflect
 * information about the last known RTH trade until the next RTH trade happens.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `Trade` and `Trade&X`
 * for regional exchange trades.
 * Regional records do not explicitly store a field for @ref Trade::getExchangeCode() "exchangeCode" property.
 */
class DXFCPP_EXPORT Trade final : public TradeBase {
    friend struct EventMapper;

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the Trade object
    using Ptr = std::shared_ptr<Trade>;

    /// The alias to a type of unique pointer to the Trade object
    using Unique = std::unique_ptr<Trade>;

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
    Trade() noexcept = default;

    /**
     * Creates new trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Trade(std::string eventSymbol) noexcept : TradeBase(std::move(eventSymbol)) {
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