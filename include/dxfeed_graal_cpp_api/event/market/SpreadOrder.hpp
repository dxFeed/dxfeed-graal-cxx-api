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
 * order @ref SpreadOrder::getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()` inclusive.
 * Use @ref SpreadOrder::setSource() "setSource" method after @ref SpreadOrder::setIndex() "setIndex" to properly
 * include source identifier into the index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref SpreadOrder::getIndex() "index", starting with
 * an event with the largest index and marking it with SpreadOrder::SNAPSHOT_BEGIN bit in @ref SpreadOrder::getEventFlags() "eventFlags",
 * and finishing the snapshot with an event that has zero 32 least significant bits of index.
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
        std::string spreadSymbol{};
    };

    SpreadOrderData spreadOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:

    static std::shared_ptr<SpreadOrder> fromGraal(void *graalNative) noexcept;

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
    /// The alias to a type of shared pointer to the SpreadOrder object
    using Ptr = std::shared_ptr<SpreadOrder>;

    /// The alias to a type of unique pointer to the SpreadOrder object
    using Unique = std::unique_ptr<SpreadOrder>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

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
     * Returns spread symbol of this event.
     *
     * @return spread symbol of this event.
     */
    const std::string &getSpreadSymbol() const & noexcept {
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
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp