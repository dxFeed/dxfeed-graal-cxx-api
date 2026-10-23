// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../market/Trade.hpp"

#include <cstdint>
#include <memory>
#include <string>

/**
 * \addtogroup dxfcpp_custom
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Represents an extension of Trade for the symbols traded on the Nuam Exchange.
 * It is the new Regional Holding that integrates the Santiago, Lima, and Colombia Stock Exchanges into a single market.
 *
 * It includes the most recent information about Nuam trade statistics published after every trade for
 * the current trading day such as number of trades, volume-weighted average price, etc.
 *
 * This is an experimental API, it may change in future versions.
 *
 * Java: <a href="https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/custom/NuamTrade.html">NuamTrade</a>
 *
 * <h3>Properties</h3>
 *
 * NuamTrade event has all the properties of Trade and the following ones:
 *
 * <ul>
 * <li>@ref NuamTrade::getTradeStatTime() "tradeStatTime" - time when trade statistics information was updated;
 * <li>@ref NuamTrade::getLastSignificantPrice() "lastSignificantPrice" - price of the last trade reported as
 * significant by the exchange;
 * <li>@ref NuamTrade::getLastPriceForAll() "lastPriceForAll" - price of the last trade (for all deals) reported by the
 * exchange;
 * <li>@ref NuamTrade::getNumberOfTrades() "numberOfTrades" - number of trades reported by the exchange;
 * <li>@ref NuamTrade::getVWAP() "vwap" - volume-weighted average price (VWAP) reported by the exchange.
 * </ul>
 *
 * <h3>Nuam trade statistics details</h3>
 *
 * All extended fields beyond the Trade represent Nuam trade statistics.
 * These statistics are sent by the exchange in a separate message following each trade message.
 * For this reason, these fields are updated independently of last trade information.
 * Use @ref NuamTrade::getTradeStatTime() "tradeStatTime" and @ref NuamTrade::getTime() "time" to distinguish
 * between last trade and trade statistics updates.
 *
 * <p>All Nuam trade statistics fields are populated using exchange data from Trade statistics messages.
 * While @ref NuamTrade::getPrice() "price" of Trade is expected to be identical to either
 * @ref NuamTrade::getLastPriceForAll() "lastPriceForAll" or
 * @ref NuamTrade::getLastSignificantPrice() "lastSignificantPrice",
 * we added both fields to separate values from the exchange from our business logic of Trade event.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `NuamTrade`.
 */
class DXFCPP_EXPORT NuamTrade final : public Trade {
    friend struct EventMapper;

    struct NuamTradeData {
        std::int64_t tradeStatTime{};
        double lastSignificantPrice = math::NaN;
        double lastPriceForAll = math::NaN;
        std::int32_t numberOfTrades{};
        double vwap = math::NaN;
    };

    NuamTradeData nuamTradeData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    public:
    /// The alias to a type of shared pointer to the NuamTrade object
    using Ptr = std::shared_ptr<NuamTrade>;

    /// The alias to a type of unique pointer to the NuamTrade object
    using Unique = std::unique_ptr<NuamTrade>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
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

    /// Creates new Nuam trade event with default values.
    NuamTrade() noexcept;

    /**
     * Creates new Nuam trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit NuamTrade(const StringLike &eventSymbol) noexcept;

    /**
     * Returns time when trade statistics information was updated. It can be used to distinguish
     * between last trade and trade statistics updates.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time when trade statistics information was updated.
     */
    std::int64_t getTradeStatTime() const noexcept;

    /**
     * Changes time when trade statistics information was updated.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param tradeStatTime time when trade statistics information was updated.
     */
    void setTradeStatTime(std::int64_t tradeStatTime) noexcept;

    /**
     * Returns price of the last trade reported as significant by the exchange.
     * For example, a trade cannot be considered significant if its price below 1000 USD.
     *
     * @return price of the last trade reported as significant by the exchange.
     */
    double getLastSignificantPrice() const noexcept;

    /**
     * Changes price of the last trade reported as significant by the exchange.
     *
     * @param lastSignificantPrice price of the last trade reported as significant by the exchange.
     */
    void setLastSignificantPrice(double lastSignificantPrice) noexcept;

    /**
     * Returns price of the last trade (for all deals) reported by the exchange.
     * Unlike @ref NuamTrade::getLastSignificantPrice() "lastSignificantPrice", it represents the last trade price
     * without any conditions.
     *
     * @return price of the last trade (for all deals) reported by the exchange.
     */
    double getLastPriceForAll() const noexcept;

    /**
     * Changes price of the last trade (for all deals) reported by the exchange.
     *
     * @param lastPriceForAll price of the last trade (for all deals) reported by the exchange.
     */
    void setLastPriceForAll(double lastPriceForAll) noexcept;

    /**
     * Returns number of trades reported by the exchange.
     *
     * @return number of trades reported by the exchange.
     */
    std::int32_t getNumberOfTrades() const noexcept;

    /**
     * Changes number of trades reported by the exchange.
     *
     * @param numberOfTrades number of trades reported by the exchange.
     */
    void setNumberOfTrades(std::int32_t numberOfTrades) noexcept;

    /**
     * Returns volume-weighted average price (VWAP) reported by the exchange.
     *
     * @return volume-weighted average price (VWAP) reported by the exchange.
     */
    double getVWAP() const noexcept;

    /**
     * Changes volume-weighted average price (VWAP) reported by the exchange.
     *
     * @param vwap volume-weighted average price (VWAP) reported by the exchange.
     */
    void setVWAP(double vwap) noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
