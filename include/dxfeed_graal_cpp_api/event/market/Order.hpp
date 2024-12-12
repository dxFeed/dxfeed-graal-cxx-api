// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

#include "OrderBase.hpp"

DXFCPP_BEGIN_NAMESPACE

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
 * The event with @ref Order::getSize() "size" either `0` or `NaN` is a signal to remove previously received order for
 * the corresponding index. The method Order::hasSize() is a convenient method to test for size presence.
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
 * order @ref Order::getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()`
 * inclusive. Use Order::setSource() method after Order::setIndex() to properly include source identifier into the
 * index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref Order::getIndex() "index", starting with
 * an event with the largest index and marking it with EventFlag::SNAPSHOT_BEGIN bit in @ref Order::getEventFlags()
 * "eventFlags", and finishing the snapshot with an event that has zero 32 least significant bits of index.
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
        std::optional<std::string> marketMaker{};
    };

    OrderData orderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the Order object
    using Ptr = std::shared_ptr<Order>;

    /// The alias to a type of unique pointer to the Order object
    using Unique = std::unique_ptr<Order>;

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

    /// Creates new order event with default values.
    Order() noexcept = default;

    /**
     * Creates new order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Order(std::string eventSymbol) noexcept : OrderBase(std::move(eventSymbol)) {
    }

    // MarketEvent methods

    /**
     * Changes event's symbol and returns the current order.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current order.
     */
    virtual Order &withEventSymbol(const std::string &eventSymbol) noexcept {
        MarketEvent::setEventSymbol(eventSymbol);

        return *this;
    }

    /**
     * Changes event's creation time and returns the current order.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current order.
     */
    Order &withEventTime(std::int64_t eventTime) noexcept {
        MarketEvent::setEventTime(eventTime);

        return *this;
    }

    // OrderBase methods

    /**
     * Changes event's source and returns the current order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source source of this event.
     * @return The current order.
     */
    Order &withSource(const OrderSource &source) noexcept {
        OrderBase::setSource(source);

        return *this;
    }

    /**
     * Changes transactional event flags and returns the current order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current order.
     */
    Order &withEventFlags(std::int32_t eventFlags) noexcept {
        OrderBase::setEventFlags(eventFlags);

        return *this;
    }

    /**
     * Changes transactional event flags and returns the current order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current order.
     */
    Order &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        OrderBase::setEventFlags(eventFlags);

        return *this;
    }

    /**
     * Changes unique per-symbol index of this order and returns it. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index unique per-symbol index of this order.
     * @return The current order.
     */
    Order &withIndex(std::int64_t index) noexcept {
        OrderBase::setIndex(index);

        return *this;
    }

    /**
     * Changes time of this order and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this order.
     * @return The current order.
     */
    Order &withTime(std::int64_t time) noexcept {
        OrderBase::setTime(time);

        return *this;
    }

    /**
     * Changes microseconds and nanoseconds time part of this order.
     * Returns the current order.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this order.
     * @return The current order.
     */
    Order &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        OrderBase::setTimeNanoPart(timeNanoPart);

        return *this;
    }

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this order.
     * Returns the current order.
     *
     * @param sequence the sequence.
     * @return The current order.
     * @see OrderBase::getSequence()
     */
    Order &withSequence(std::int32_t sequence) noexcept {
        OrderBase::setSequence(sequence);

        return *this;
    }

    /**
     * Changes time of this order and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this order in nanoseconds.
     * @return The current order.
     */
    Order &withTimeNanos(std::int64_t timeNanos) noexcept {
        OrderBase::setTimeNanos(timeNanos);

        return *this;
    }

    /**
     * Changes action of this order and returns it.
     *
     * @param action The side of this order.
     * @return The current order.
     */
    Order &withAction(const OrderAction &action) noexcept {
        OrderBase::setAction(action);

        return *this;
    }

    /**
     * Changes time of the last action and returns current order.
     *
     * @param actionTime The last order action time.
     * @return The current order.
     */
    Order &withActionTime(std::int64_t actionTime) noexcept {
        OrderBase::setActionTime(actionTime);

        return *this;
    }

    /**
     * Changes order ID.
     * Returns the current order.
     *
     * @param orderId The order ID.
     * @return The current order.
     */
    Order &withOrderId(std::int64_t orderId) noexcept {
        OrderBase::setOrderId(orderId);

        return *this;
    }

    /**
     * Changes auxiliary order ID.
     * Returns the current order.
     *
     * @param auxOrderId The auxiliary order ID.
     * @return The current order.
     */
    Order &withAuxOrderId(std::int64_t auxOrderId) noexcept {
        OrderBase::setAuxOrderId(auxOrderId);

        return *this;
    }

    /**
     * Changes price of this order.
     * Returns the current order.
     *
     * @param price The price of this order.
     * @return The current order.
     */
    Order &withPrice(double price) noexcept {
        OrderBase::setPrice(price);

        return *this;
    }

    /**
     * Changes size of this order.
     * Returns the current order.
     *
     * @param size The size of this order.
     * @return The current order.
     */
    Order &withSize(double size) noexcept {
        OrderBase::setSize(size);

        return *this;
    }

    /**
     * Changes executed size of this order.
     * Returns the current order.
     *
     * @param executedSize The executed size of this order.
     * @return The current order.
     */
    Order &withExecutedSize(double executedSize) noexcept {
        OrderBase::setExecutedSize(executedSize);

        return *this;
    }

    /**
     * Changes number of individual orders in this aggregate order.
     * Returns the current order.
     *
     * @param count The number of individual orders in this aggregate order.
     * @return The current order.
     */
    Order &withCount(std::int64_t count) noexcept {
        OrderBase::setCount(count);

        return *this;
    }

    /**
     * Changes trade ID.
     * Returns the current order.
     *
     * @param tradeId The trade ID.
     * @return The current order.
     */
    Order &withTradeId(std::int64_t tradeId) noexcept {
        OrderBase::setTradeId(tradeId);

        return *this;
    }

    /**
     * Changes trade price.
     * Returns the current order.
     *
     * @param tradePrice The trade price.
     * @return The current order.
     */
    Order &withTradePrice(double tradePrice) noexcept {
        OrderBase::setTradePrice(tradePrice);

        return *this;
    }

    /**
     * Changes trade size.
     * Returns the current order.
     *
     * @param tradeSize The trade size.
     * @return The current order.
     */
    Order &withTradeSize(double tradeSize) noexcept {
        OrderBase::setTradeSize(tradeSize);

        return *this;
    }

    /**
     * Changes exchange code of this order.
     * Returns the current order.
     *
     * @param exchangeCode The exchange code of this order.
     * @return The current order.
     */
    Order &withExchangeCode(char exchangeCode) noexcept {
        OrderBase::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Changes exchange code of this order.
     * Returns the current order.
     *
     * @param exchangeCode The exchange code of this order.
     * @return The current order.
     */
    Order &withExchangeCode(std::int16_t exchangeCode) noexcept {
        OrderBase::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Changes side of this order.
     * Returns the current order.
     *
     * @param side The side of this order.
     * @return The current order.
     */
    Order &withOrderSide(const Side &side) noexcept {
        OrderBase::setOrderSide(side);

        return *this;
    }

    /**
     * Changes scope of this order.
     * Returns the current order.
     *
     * @param scope The scope of this order.
     * @return The current order.
     */
    Order &withScope(const Scope &scope) noexcept {
        OrderBase::setScope(scope);

        return *this;
    }

    // Order methods

    /**
     * Returns market maker or other aggregate identifier of this order.
     * This value is defined for Scope::AGGREGATE and Scope::ORDER orders.
     *
     * @return market maker or other aggregate identifier of this order or dxfcpp::String::NUL
     * (`std::string{"<null>"}`).
     */
    const std::string &getMarketMaker() const & noexcept {
        if (!orderData_.marketMaker) {
            return dxfcpp::String::NUL;
        }

        return orderData_.marketMaker.value();
    }

    /**
     * Returns market maker or other aggregate identifier of this order.
     * This value is defined for Scope::AGGREGATE and Scope::ORDER orders.
     *
     * @return market maker or other aggregate identifier of this order or `std::nullopt`.
     */
    const std::optional<std::string> &getMarketMakerOpt() const & noexcept {
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
     * Changes market maker or other aggregate identifier of this order.
     * Returns the current order.
     *
     * @param marketMaker The market maker or other aggregate identifier of this order.
     * @return The current order.
     */
    Order &withMarketMaker(std::string marketMaker) noexcept {
        setMarketMaker(std::move(marketMaker));

        return *this;
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