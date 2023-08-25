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

#include "OrderBase.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Order event is a snapshot for a full available market depth for a symbol.
 * The collection of order events of a symbol represents the most recent information
 * that is available about orders on the market at any given moment of time.
 * Order events give information on several levels of details, called scopes - see Scope.
 * Scope of an order is available via @ref Order::getScope "scope" property.
 *
 * <p> Order events arrive from
 * multiple sources for the same market symbol and are distinguished by their @ref Order::getIndex "index". Index is a
 * unique per symbol identifier of the event.
 * It is unique across all the sources of depth information for the symbol.
 * The event with @ref Order::getSize() "size" either `0` or `NaN` is a signal to remove previously received order for the
 * corresponding index.
 * The method Order::hasSize() is a convenient method to test for size presence.
 *
 * <p> Events from finer-grained Scope of detail give more information and include events
 * from coarse-grained Scope of detail. For a consistent representation of the market depth
 * to the end-user, order events must be filtered to remove coarse-grained events that
 * are superseded by the finer-grained events.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some order event sources provide a consistent view of the price-level or detailed order book. Their updates
 * may incorporate multiple changes to price levels or to individual orders that have to be processed at the same time.
 * The corresponding information is carried in @ref Order::getEventFlags() "eventFlags" property.
 *
 * <p> See `Event Flags` section of OrderBase class documentation for details.
 *
 * <p> The composite quotes with Scope::COMPOSITE and regional quotes with Scope::REGIONAL come
 * individually from different venues and are not related to each other in any transactional way. The result of
 * Order::getEventFlags method for them is always zero.
 *
 * <h3>Publishing order books</h3>
 *
 * When publishing an order event with DXPublisher::publishEvents() method, least significant 32 bits of
 * order @ref Order::getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()` inclusive.
 * Use Order::setSource() method after Order::setIndex() to properly include source identifier into the index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref Order::getIndex() "index", starting with
 * an event with the largest index and marking it with EventFlag::SNAPSHOT_BEGIN bit in @ref Order::getEventFlags() "eventFlags",
 * and finishing the snapshot with an event that has zero 32 least significant bits of index.
 * EventFlag::SNAPSHOT_END bit in @ref Order::getEventFlags() "eventFlags" is optional during publishing.
 * It will be properly set on receiving end anyway.
 *
 * <h3>Limitations</h3>
 *
 * This event type cannot be used with DXFeed::getLastEvent() method.
 *
 * <h3><a name="fobSection">Full Order Book Support</a></h3>
 *
 * Some feeds provide support for "Full Order Book" (FOB) where additional fields will be available:
 * <ul>
 * <li>@ref Order::getAction() "action" - event business meaning (see OrderAction for more details)</li>
 * <li>@ref Order::getActionTime() "actionTime" - time of the last action</li>
 * <li>@ref Order::getOrderId() "orderId" - ID of this order</li>
 * <li>@ref Order::getAuxOrderId() "auxOrderId" - additional ID for this order</li>
 * <li>@ref Order::getTradeId() "tradeId" - trade (order execution) ID</li>
 * <li>@ref Order::getTradePrice() "tradePrice" - price of the trade</li>
 * <li>@ref Order::getTradeSize() "tradeSize" - size of the trade</li>
 * </ul>
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `Quote` for composite quotes with Scope::COMPOSITE,
 * record `Quote&X` for regional exchange best quotes with Scope::REGIONAL,
 * record `MarketMaker` for market-maker quotes and futures price level aggregates with Scope::AGGREGATE,
 * record `Order` for the most fine-grained Scope::ORDER with zero source id,
 * and records `Order#<source-id>` for specific source ids,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "NTV".
 */
class DXFCPP_EXPORT Order : public OrderBase {
    friend struct EventMapper;

  protected:
    struct OrderData {
        std::string marketMaker{};
    };

    OrderData orderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:

    static std::shared_ptr<Order> fromGraal(void *graalNative) noexcept;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void* toGraal() const noexcept override;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     */
    static void freeGraal(void* graalNative) noexcept;

  public:
    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /// Creates new order event with default values.
    Order() noexcept = default;

    /**
     * Creates new order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Order(std::string eventSymbol) noexcept : OrderBase(std::move(eventSymbol)) {
    }

    /**
     * Returns market maker or other aggregate identifier of this order.
     * This value is defined for Scope::AGGREGATE and Scope::ORDER orders.
     *
     * @return market maker or other aggregate identifier of this order.
     */
    const std::string &getMarketMaker() const & noexcept {
        return orderData_.marketMaker;
    }

    /**
     * Changes market maker or other aggregate identifier of this order.
     *
     * @param marketMaker market maker or other aggregate identifier of this order.
     */
    void setMarketMaker(std::string marketMaker) noexcept {
        orderData_.marketMaker = std::move(marketMaker);
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp