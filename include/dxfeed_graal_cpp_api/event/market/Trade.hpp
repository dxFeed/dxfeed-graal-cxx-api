// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "TradeBase.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Trade event is a snapshot of the price and size of the last trade during regular trading hours
 * and an overall day volume and day turnover.
 * It represents the most recent information that is available about the regular last trade on the market
 * at any given moment of time.
 *
 * <h3>Trading sessions</h3>
 *
 * The Trade event defines last trade @ref ::getPrice() "price" as officially defined
 * by the corresponding exchange for its <b>regular trading hours</b> (RTH).
 * It also include an official exchange @ref ::getDayVolume() "dayVolume" and @ref ::getDayTurnover() "dayTurnover"
 * <b>for the whole trading day</b> identified by @ref ::getDayId() "dayId".
 * So, Trade event captures all the official numbers that are typically reported by exchange.
 *
 * <p>Trades that happen in <b>extended trading hours</b> (ETH, pre-market and post-market trading sessions),
 * which are typically defined for stocks and ETFs, do not update last trade @ref ::getTime() "time",
 * @ref ::getExchangeCode() "exchangeCode", @ref ::getPrice() "price", @ref ::getChange() "change",
 * @ref ::getSize() "size", and @ref ::getTickDirection() "tickDirection" in the Trade event, but they do update
 * @ref ::getDayVolume() "dayVolume" and @ref ::getDayTurnover() "dayTurnover".
 *
 * <p>During extended trading hours a TradeETH event is generated on each trade with its
 * @ref TradeETH::isExtendedTradingHours() "extendedTradingHours" property set to `true`.</p>
 *
 * <h3>Volume and Turnover</h3>
 *
 * <p>The volume and turnover are included into the Trade event instead
 * of Summary event, because both volume and turnover typically update with each trade.
 * The @ref ::getDayId() "dayId" field identifies current trading day for which volume and turnover statistics are computed.
 * This solution avoids generation of multiple events on each trade during regular trading hours.
 * Summary event is generated during the trading day only when new highs or lows are reached or other properties change.
 *
 * <p>Note that one can compute volume-weighted average price (VWAP) for a day by this formula:
 * <br><code>vwap = @ref ::getDayTurnover() "dayTurnover" / @ref ::getDayVolume() "dayVolume";</code>
 *
 * <h3>Daily reset</h3>
 *
 * Daily reset procedure that happens on a schedule during non-trading hours resets Trade
 * @ref ::getDayVolume() "dayVolume" and @ref ::getDayTurnover() "dayTurnover" to math::NaN
 * and sets @ref ::getDayId() "dayId" to the next trading day in preparation to the next day's pre-market trading session
 * (or for regular trading if there is no pre-market) while leaving all other properties intact.
 * They reflect information about the last known RTH trade until the next RTH trade happens.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `Trade` and `Trade&X`
 * for regional exchange trades.
 * Regional records do not explicitly store a field for @ref #getExchangeCode() "exchangeCode" property.
 */
class DXFCPP_EXPORT Trade final : public TradeBase {
    friend struct EventMapper;

    static std::shared_ptr<Trade> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new trade event with default values.
    Trade() noexcept = default;

    /**
     * Creates new trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Trade(std::string eventSymbol) noexcept : TradeBase(std::move(eventSymbol)) {}

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp