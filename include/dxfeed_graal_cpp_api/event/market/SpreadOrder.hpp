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
 * Spread order event is a snapshot for a full available market depth for all spreads
 * on a given underlying symbol. The collection of spread order events of a symbol
 * represents the most recent information that is available about spread orders on
 * the market at any given moment of time.
 *
 * <p> Spread order is similar to a regular Order, but it has a
 * @ref SpreadOrder::getSpreadSymbol() "spreadSymbol" property that contains the symbol
 * of the actual spread that is being represented by spread order object.
 * @ref SpreadOrder::getEventSymbol() "eventSymbol" property contains the underlying symbol
 * that was used in subscription.
 *
 * <p> Like regular orders, spread order events arrive from
 * multiple sources for the same market symbol and are distinguished by their
 * @ref SpreadOrder::getIndex "index".
 * It is unique across all the sources of depth information for the symbol.
 * The event with @ref SpreadOrder::getSize() "size" either `0` or `NaN`
 * is a signal to remove previously received order for the corresponding index.
 * The method @ref SpreadOrder::hasSize() "hasSize" is a convenient method to test for size presence.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some spread order event sources provide a consistent view of the price-level or detailed order book. Their updates
 * may incorporate multiple changes to price levels or to individual orders that have to be processed at the same time.
 * The corresponding information is carried in @ref SpreadOrder::getEventFlags() "eventFlags" property.
 *
 * <p> See `Event Flags` section of OrderBase class documentation for details.
 *
 * <p>IndexedEventModel class handles all the snapshot and transaction logic and conveniently represents
 * a list current of events.
 * It relies on the code of AbstractIndexedEventModel to handle this logic.
 * Use the source code of AbstractIndexedEventModel for clarification on transactions and snapshot logic.
 *
 * <h3>Publishing order books</h3>
 *
 * When publishing an order event with DXPublisher::publishEvents() method, least significant 32 bits of
 * order @ref SpreadOrder::getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()`
 * inclusive. Use @ref SpreadOrder::setSource() "setSource" method after @ref SpreadOrder::setIndex() "setIndex" to
 * properly include source identifier into the index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref SpreadOrder::getIndex() "index", starting
 * with an event with the largest index and marking it with SpreadOrder::SNAPSHOT_BEGIN bit in @ref
 * SpreadOrder::getEventFlags() "eventFlags", and finishing the snapshot with an event that has zero 32 least
 * significant bits of index.
 * @ref SpreadOrder::SNAPSHOT_END bit in @ref SpreadOrder::getEventFlags() "eventFlags" is optional during publishing.
 * It will be properly set on receiving end anyway.
 *
 * <h3>Limitations</h3>
 *
 * This event type cannot be used with @ref DXFeed::getLastEvent() method.
 *
 * <h3><a name="fobSection">Full Order Book Support</a></h3>
 *
 * Some feeds provide support for "Full Order Book" (FOB) where additional fields will be available:
 * <ul>
 * <li>@ref SpreadOrder::getAction() "action" - event business meaning (see OrderAction for more details)</li>
 * <li>@ref SpreadOrder::getActionTime() "actionTime" - time of the last action</li>
 * <li>@ref SpreadOrder::getOrderId() "orderId" - ID of this order</li>
 * <li>@ref SpreadOrder::getAuxOrderId() "auxOrderId" - additional ID for this order</li>
 * <li>@ref SpreadOrder::getTradeId() "tradeId" - trade (order execution) ID</li>
 * <li>@ref SpreadOrder::getTradePrice() "tradePrice" - price of the trade</li>
 * <li>@ref SpreadOrder::getTradeSize() "tradeSize" - size of the trade</li>
 * </ul>
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `SpreadOrder#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "ISE".
 */
class DXFCPP_EXPORT SpreadOrder : public OrderBase {
    friend struct EventMapper;

  protected:
    struct SpreadOrderData {
        std::optional<std::string> spreadSymbol{};
    };

    SpreadOrderData spreadOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the SpreadOrder object
    using Ptr = std::shared_ptr<SpreadOrder>;

    /// The alias to a type of unique pointer to the SpreadOrder object
    using Unique = std::unique_ptr<SpreadOrder>;

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

    /// Creates new spread order event with default values.
    SpreadOrder() noexcept = default;

    /**
     * Creates new spread order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit SpreadOrder(std::string eventSymbol) noexcept : OrderBase(std::move(eventSymbol)) {
    }

    /**
     * Changes event's symbol and returns the current spread order.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current spread order.
     */
    SpreadOrder &withEventSymbol(const std::string &eventSymbol) noexcept {
        MarketEvent::setEventSymbol(eventSymbol);

        return *this;
    }

    /**
     * Changes event's creation time and returns the current spread order.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current spread order.
     */
    SpreadOrder &withEventTime(std::int64_t eventTime) noexcept {
        MarketEvent::setEventTime(eventTime);

        return *this;
    }

    /**
     * Changes event's source and returns the current spread order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source The source of this event.
     * @return The current spread order.
     */
    SpreadOrder &withSource(const OrderSource &source) noexcept {
        OrderBase::setSource(source);

        return *this;
    }

    /**
     * Changes transactional event flags and returns the current spread order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags The transactional event flags.
     * @return The current spread order.
     */
    SpreadOrder &withEventFlags(std::int32_t eventFlags) noexcept {
        OrderBase::setEventFlags(eventFlags);

        return *this;
    }

    /**
     * Changes transactional event flags and returns the current spread order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags The transactional event flags' mask.
     * @return The current spread order.
     */
    SpreadOrder &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        OrderBase::setEventFlags(eventFlags);

        return *this;
    }

    /**
     * Changes unique per-symbol index of this spread order and returns it. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index The unique per-symbol index of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withIndex(std::int64_t index) noexcept {
        OrderBase::setIndex(index);

        return *this;
    }

    /**
     * Changes time of this spread order and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time The time of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withTime(std::int64_t time) noexcept {
        OrderBase::setTime(time);

        return *this;
    }

    /**
     * Changes microseconds and nanoseconds time part of this spread order.
     * Returns the current spread order.
     *
     * @param timeNanoPart The microseconds and nanoseconds time part of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        OrderBase::setTimeNanoPart(timeNanoPart);

        return *this;
    }

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this spread order.
     * Returns the current spread order.
     *
     * @param sequence The sequence.
     * @return The current spread order.
     * @see OrderBase::getSequence()
     */
    SpreadOrder &withSequence(std::int32_t sequence) noexcept {
        OrderBase::setSequence(sequence);

        return *this;
    }

    /**
     * Changes time of this spread order and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this order in nanoseconds.
     * @return The current spread order.
     */
    SpreadOrder &withTimeNanos(std::int64_t timeNanos) noexcept {
        OrderBase::setTimeNanos(timeNanos);

        return *this;
    }

    /**
     * Changes action of this spread order and returns it.
     *
     * @param action The side of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withAction(const OrderAction &action) noexcept {
        OrderBase::setAction(action);

        return *this;
    }

    /**
     * Changes time of the last action and returns current spread order.
     *
     * @param actionTime The last spread order action time.
     * @return The current spread order.
     */
    SpreadOrder &withActionTime(std::int64_t actionTime) noexcept {
        OrderBase::setActionTime(actionTime);

        return *this;
    }

    /**
     * Changes order ID.
     * Returns the current spread order.
     *
     * @param orderId The spread order ID.
     * @return The current spread order.
     */
    SpreadOrder &withOrderId(std::int64_t orderId) noexcept {
        OrderBase::setOrderId(orderId);

        return *this;
    }

    /**
     * Changes auxiliary spread order ID.
     * Returns the current spread order.
     *
     * @param auxOrderId The auxiliary spread order ID.
     * @return The current spread order.
     */
    SpreadOrder &withAuxOrderId(std::int64_t auxOrderId) noexcept {
        OrderBase::setAuxOrderId(auxOrderId);

        return *this;
    }

    /**
     * Changes price of this spread order.
     * Returns the current spread order.
     *
     * @param price The price of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withPrice(double price) noexcept {
        OrderBase::setPrice(price);

        return *this;
    }

    /**
     * Changes size of this spread order.
     * Returns the current spread order.
     *
     * @param size The size of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withSize(double size) noexcept {
        OrderBase::setSize(size);

        return *this;
    }

    /**
     * Changes executed size of this spread order.
     * Returns the current spread order.
     *
     * @param executedSize The executed size of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withExecutedSize(double executedSize) noexcept {
        OrderBase::setExecutedSize(executedSize);

        return *this;
    }

    /**
     * Changes number of individual spread orders in this aggregate spread order.
     * Returns the current spread order.
     *
     * @param count The number of individual orders in this aggregate spread order.
     * @return The current spread order.
     */
    SpreadOrder &withCount(std::int64_t count) noexcept {
        OrderBase::setCount(count);

        return *this;
    }

    /**
     * Changes trade ID.
     * Returns the current spread order.
     *
     * @param tradeId The trade ID.
     * @return The current spread order.
     */
    SpreadOrder &withTradeId(std::int64_t tradeId) noexcept {
        OrderBase::setTradeId(tradeId);

        return *this;
    }

    /**
     * Changes trade price.
     * Returns the current spread order.
     *
     * @param tradePrice The trade price.
     * @return The current spread order.
     */
    SpreadOrder &withTradePrice(double tradePrice) noexcept {
        OrderBase::setTradePrice(tradePrice);

        return *this;
    }

    /**
     * Changes trade size.
     * Returns the current spread order.
     *
     * @param tradeSize The trade size.
     * @return The current spread order.
     */
    SpreadOrder &withTradeSize(double tradeSize) noexcept {
        OrderBase::setTradeSize(tradeSize);

        return *this;
    }

    /**
     * Changes exchange code of this spread order.
     * Returns the current spread order.
     *
     * @param exchangeCode The exchange code of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withExchangeCode(char exchangeCode) noexcept {
        OrderBase::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Changes exchange code of this spread order.
     * Returns the current spread order.
     *
     * @param exchangeCode The exchange code of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withExchangeCode(std::int16_t exchangeCode) noexcept {
        OrderBase::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Changes side of this spread order.
     * Returns the current spread order.
     *
     * @param side The side of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withOrderSide(const Side &side) noexcept {
        OrderBase::setOrderSide(side);

        return *this;
    }

    /**
     * Changes scope of this spread order.
     * Returns the current spread order.
     *
     * @param scope The scope of this spread order.
     * @return The current spread order.
     */
    SpreadOrder &withScope(const Scope &scope) noexcept {
        OrderBase::setScope(scope);

        return *this;
    }

    /**
     * Returns spread symbol of this event.
     *
     * @return spread symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getSpreadSymbol() const & noexcept {
        if (!spreadOrderData_.spreadSymbol) {
            return dxfcpp::String::NUL;
        }

        return spreadOrderData_.spreadSymbol.value();
    }

    /**
     * Returns spread symbol of this event.
     *
     * @return spread symbol of this event or std::nullopt
     */
    const std::optional<std::string> &getSpreadSymbolOpt() const & noexcept {
        return spreadOrderData_.spreadSymbol;
    }

    /**
     * Changes spread symbol of this event.
     *
     * @param spreadSymbol spread symbol of this event.
     */
    void setSpreadSymbol(std::string spreadSymbol) noexcept {
        spreadOrderData_.spreadSymbol = std::move(spreadSymbol);
    }

    /**
     * Changes spread symbol of this event.
     * Returns the current spread order.
     *
     * @param spreadSymbol spread symbol of this event.
     * @return The current spread order.
     */
    SpreadOrder &withSpreadSymbol(std::string spreadSymbol) noexcept {
        setSpreadSymbol(std::move(spreadSymbol));

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