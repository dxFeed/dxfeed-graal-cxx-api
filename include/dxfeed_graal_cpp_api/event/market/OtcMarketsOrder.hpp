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

#include "Order.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Order event is a snapshot for a full available market depth for a symbol.
 * The collection of order events of a symbol represents the most recent information
 * that is available about orders on the market at any given moment of time.
 * Order events give information on several levels of details, called scopes - see {@link Scope}.
 * Scope of an order is available via {@link #getScope scope} property.
 *
 * <p> Order events arrive from
 * multiple sources for the same market symbol and are distinguished by their
 * {@link #getIndex index}. Index is a unique per symbol identifier of the event.
 * It is unique across all the sources of depth information for the symbol.
 * The event with {@link #getSizeAsDouble() sizeAsDouble} either {@code 0} or {@link Double#NaN NaN}
 * is a signal to remove previously received order for the corresponding index.
 * The method {@link #hasSize() hasSize} is a convenient method to test for size presence.
 *
 * <p> Events from finer-grained {@link Scope} of detail give more information and include events
 * from coarse-grained {@link Scope} of detail. For a consistent representation of the market depth
 * to the end-user, order events must be filtered to remove coarse-grained events that
 * are superseded by the finer-grained events.
 *
 * <h3>Properties</h3>
 *
 * {@code Order} event has the following properties:
 *
 * <ul>
 * <li>{@link #getEventSymbol() eventSymbol} - symbol of this event;
 * <li>{@link #getSource() source} - source of this event;
 * <li>{@link #getEventFlags() eventFlags} - transactional event flags;
 * <li>{@link #getIndex() index} - unique per-symbol index of this order;
 * <li>{@link #getTime() time} - time of this order;
 * <li>{@link #getTimeNanoPart() timeNanoPart} - microseconds and nanoseconds time part of this order;
 * <li>{@link #getSequence() sequence} - sequence of this order;
 * <li>{@link #getPrice() price} - price of this order;
 * <li>{@link #getSize() size} - size of this order as integer number (rounded toward zero);
 * <li>{@link #getSizeAsDouble() sizeAsDouble} - size of this order as floating number with fractions;
 * <li>{@link #getExecutedSize()} - executed size of this order;
 * <li>{@link #getCount() count} - number of individual orders in this aggregate order;
 * <li>{@link #getExchangeCode() exchangeCode} - exchange code of this order;
 * <li>{@link #getOrderSide() orderSide} - side of this order;
 * <li>{@link #getScope() scope} - scope of this order;
 * <li>{@link #getMarketMaker() marketMaker} - market maker or other aggregate identifier of this order.
 * </ul>
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some order event sources provide a consistent view of the price-level or detailed order book. Their updates
 * may incorporate multiple changes to price levels or to individual orders that have to be processed at the same time.
 * The corresponding information is carried in {@link #getEventFlags() eventFlags} property.
 *
 * <p> See <a href="OrderBase.html#eventFlagsSection">Event Flags</a> section of {@link OrderBase}
 * class documentation for details.
 *
 * <p> The composite quotes with {@link Scope#COMPOSITE} and regional quotes with {@link Scope#REGIONAL} come
 * individually from different venues and are not related to each other in any transactional way. The result of
 * {@link #getEventFlags() getEventFlags} method for them is always zero.
 *
 * <p> {@link OrderBookModel} class contains all the appropriate logic to deal with transactions and snapshots
 * for {@link Order} events. The client-visible changes to the model are reported only when the snapshot for the
 * specific source id is received completely and when there is no ongoing transaction for the specific source id.
 * It relies on the code of {@link AbstractIndexedEventModel} to handle this logic.
 * Use the source code of {@link AbstractIndexedEventModel} for clarification on transactions and snapshot logic.
 *
 * <h3>Publishing order books</h3>
 *
 * When publishing an order event with {@link DXPublisher#publishEvents(Collection) DXPublisher.publishEvents}
 * method, least significant 32 bits of order {@link #getIndex() index} must be in a range of from 0 to
 * {@link Integer#MAX_VALUE} inclusive.
 * Use {@link #setSource(OrderSource) setSource} method after {@link #setIndex(long) setIndex} to properly
 * include source identifier into the index.
 *
 * A snapshot has to be published in the <em>descending</em> order of {@link #getIndex() index}, starting with
 * an event with the largest index and marking it with {@link #SNAPSHOT_BEGIN} bit in {@link #getEventFlags() eventFlags},
 * and finishing the snapshot with an event that has zero 32 least significant bits of index.
 * {@link #SNAPSHOT_END} bit in {@link #getEventFlags() eventFlags} is optional during publishing.
 * It will be properly set on receiving end anyway.
 *
 * <h3>Limitations</h3>
 *
 * This event type cannot be used with {@link DXFeed#getLastEvent DXFeed.getLastEvent} method.
 *
 * <h3><a name="fobSection">Full Order Book Support</a></h3>
 *
 * Some feeds provide support for "Full Order Book" (FOB) where additional fields will be available:
 * <ul>
 * <li>{@link #getAction() action} - event business meaning (see {@link OrderAction} for more details)</li>
 * <li>{@link #getActionTime() actionTime} - time of the last action</li>
 * <li>{@link #getOrderId() orderId} - ID of this order</li>
 * <li>{@link #getAuxOrderId() auxOrderId} - additional ID for this order</li>
 * <li>{@link #getTradeId() tradeId} - trade (order execution) ID</li>
 * <li>{@link #getTradePrice() tradePrice} - price of the trade</li>
 * <li>{@link #getTradeSize() tradeSize} - size of the trade</li>
 * </ul>
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record {@code Quote} for composite quotes with {@link Scope#COMPOSITE},
 * record {@code Quote&X} for regional exchange best quotes with {@link Scope#REGIONAL},
 * record {@code MarketMaker} for market-maker quotes and futures price level aggregates with {@link Scope#AGGREGATE},
 * record {@code Order} for the most fine-grained {@link Scope#ORDER} with zero source id,
 * and records {@code Order#<source-id>} for specific source ids,
 * where {@code <source-id>} is up to 4 ASCII characters with a mnemonic for the source like "NTV".
 */
class DXFCPP_EXPORT OtcMarketsOrder final : public Order {
    friend struct EventMapper;

    /*
     * OTC Markets flags property has several significant bits that are packed into an integer in the following way:
     *   31..7          6                5             4          3       2          1          0
     * +-------+-----------------+---------------+-----------+--------+-------+-------------+------+
     * |       | NMS Conditional | AutoExecution | Saturated | OTC Price Type | Unsolicited | Open |
     * +-------+-----------------+---------------+-----------+--------+-------+-------------+------+
     * |                Extended Quote Flags                 |             Quote Flags             |
     * +-----------------------------------------------------+-------------------------------------+
     */

    static constexpr std::uint32_t NMS_CONDITIONAL = 1 << 6;
    static constexpr std::uint32_t AUTO_EXECUTION = 1 << 5;
    static constexpr std::uint32_t SATURATED = 1 << 4;

    // OTC_PRICE_TYPE values are taken from OtcMarketsPriceType enum.
    static constexpr std::uint32_t OTC_PRICE_TYPE_MASK = 3;
    static constexpr std::uint32_t OTC_PRICE_TYPE_SHIFT = 2;

    static constexpr std::uint32_t UNSOLICITED = 1 << 1;
    static constexpr std::uint32_t OPEN = 1;

  protected:
    struct OtcMarketsOrderData {
        std::int32_t quoteAccessPayment{};
        std::int32_t otcMarketsFlags{};
    };

    OtcMarketsOrderData otcMarketsOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    static std::shared_ptr<OtcMarketsOrder> fromGraal(void *graalNative);

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
     */
    static void freeGraal(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the OtcMarketsOrder object
    using Ptr = std::shared_ptr<OtcMarketsOrder>;

    /// The alias to a type of unique pointer to the OtcMarketsOrder object
    using Unique = std::unique_ptr<OtcMarketsOrder>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /// Creates new OTC Markets order event with default values.
    OtcMarketsOrder() noexcept = default;

    /**
     * Creates new OTC Markets order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OtcMarketsOrder(std::string eventSymbol) noexcept : Order(std::move(eventSymbol)) {
    }

    /**
     * Changes event's symbol and returns the current OTC Markets order.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withEventSymbol(const std::string &eventSymbol) noexcept override {
        return dynamic_cast<OtcMarketsOrder &>(Order::withEventSymbol(eventSymbol));
    }

    /**
     * Changes event's symbol and returns a shared pointer to the current OTC Markets order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<Order>(new
     * Order(...))` or `std::make_shared<Order>(...)`
     *
     * @param eventSymbol The symbol of this event.
     * @return A shared pointer to the current order.
     */
    OtcMarketsOrder::Ptr withEventSymbolShared(const std::string &eventSymbol) noexcept {
        return Order::withEventSymbolShared(eventSymbol)->sharedAs<OtcMarketsOrder>();
    }


    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};


DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()