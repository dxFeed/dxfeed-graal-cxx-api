// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

#include "Order.hpp"
#include "OtcMarketsPriceType.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Represents an extension of Order for the symbols traded on the OTC Markets. It includes the OTC Markets specific
 * quote data, such as @ref OtcMarketsOrder::getQuoteAccessPayment() "Quote Access Payment (QAP) value",
 * @ref OtcMarketsOrder::getOtcMarketsPriceType() "Quote Price Type",
 * @ref OtcMarketsOrder::isOpen() "Open/Closed status",
 * and @ref OtcMarketsOrder::isUnsolicited() "Unsolicited quote indicator". The original event (OTC Quote) published by
 * the exchange is presented in the form of two separate OTCMarketsOrder events (bid and ask side).
 * Note that the description of the fields (e.g. QAP) uses the original "quote" term.
 * <p>
 * For more information about original fields, QAP, Quote Flags and Extended Quote Flags,
 * see <a href="https://downloads.dxfeed.com/specifications/OTC_Markets_Multicast_Data_Feeds.pdf">OTC Markets Multicast
 * Data Feed</a>. <p> For more information about display requirements, see <a
 * href="https://downloads.dxfeed.com/specifications/OTC_Markets_Data_Display_Requirements.pdf">OTC Markets Display
 * Requirements</a>.
 *
 * <p> Like regular orders, OTC Markets order events arrive from multiple sources for the same market symbol and are
 * distinguished by their @ref ::getIndex "index".
 * It is unique across all the sources of depth information for the symbol.
 * The event with @ref ::getSize() "size" either `0` or `NaN` is a signal to remove previously received order for the
 * corresponding index.
 * The method ::hasSize() is a convenient method to test for size presence.
 *
 * <h3>Quote Flags</h3>
 *
 * Quote Flags from OTC Markets are mapped to the access methods of this class as follows:
 * <ul>
 *     <li>Update Side - @ref OtcMarketsOrder::getOrderSide() "orderSide"
 *     <li>State - @ref OtcMarketsOrder::isOpen() "open"
 *     <li>Ask/Bid Unsolicited - @ref OtcMarketsOrder::isUnsolicited() "unsolicited"
 *     <li>Ask/Bid Priced, Aks/Bid OW/BW - @ref OtcMarketsOrder::getOtcMarketsPriceType() "otcMarketsPriceType"
 * </ul>
 *
 * <h3>Extended Quote Flags</h3>
 *
 * Extended Quote Flags from OTC Markets are mapped to the access methods of this class as follows:
 * <ul>
 *     <li>QuoteSaturatedFlag - @ref OtcMarketsOrder::isSaturated() "saturated"
 *     <li>Bid/Offer AutoExFlag - @ref OtcMarketsOrder::isAutoExecution() "autoExecution"
 *     <li>NMSConditionalQuoteFlag - @ref OtcMarketsOrder::isNmsConditional() "nmsConditional"
 * </ul>
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * OTC Markets order event sources provide a consistent view of the order book. Their updates
 * may incorporate multiple changes to individual orders that have to be processed at the same time.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 *
 * <p> See the "Event Flags" section of OrderBase class documentation for details.
 *
 * <h3>Publishing order books</h3>
 *
 * When publishing an order event with DXPublisher::publishEvents() method, least significant 32 bits of
 * order @ref ::getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()`
 * inclusive. Use ::setSource() method after ::setIndex() to properly include source identifier into the
 * index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref ::getIndex() "index", starting with
 * an event with the largest index and marking it with EventFlag::SNAPSHOT_BEGIN bit in @ref ::getEventFlags()
 * "eventFlags", and finishing the snapshot with an event that has zero 32 least significant bits of index.
 * EventFlag::SNAPSHOT_END bit in @ref ::getEventFlags() "eventFlags" is optional during publishing.
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
 * <li>@ref ::getAction() "action" - event business meaning (see OrderAction for more details)</li>
 * <li>@ref ::getActionTime() "actionTime" - time of the last action</li>
 * <li>@ref ::getOrderId() "orderId" - ID of this OTC Markets order</li>
 * <li>@ref ::getAuxOrderId() "auxOrderId" - additional ID for this OTC Markets order</li>
 * <li>@ref ::getTradeId() "tradeId" - trade (order execution) ID</li>
 * <li>@ref ::getTradePrice() "tradePrice" - price of the trade</li>
 * <li>@ref ::getTradeSize() "tradeSize" - size of the trade</li>
 * </ul>
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `OtcMarketsOrder#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "pink".
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

    struct OtcMarketsOrderData {
        std::int32_t quoteAccessPayment{};
        std::int32_t otcMarketsFlags{};
    };

    OtcMarketsOrderData otcMarketsOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the OtcMarketsOrder object
    using Ptr = std::shared_ptr<OtcMarketsOrder>;

    /// The alias to a type of unique pointer to the OtcMarketsOrder object
    using Unique = std::unique_ptr<OtcMarketsOrder>;

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

    /// Creates new OTC Markets order event with default values.
    OtcMarketsOrder() noexcept = default;

    /**
     * Creates new OTC Markets order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OtcMarketsOrder(std::string eventSymbol) noexcept : Order(std::move(eventSymbol)) {
    }

    // MarketEvent methods

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
     * Changes event's creation time and returns the current OTC Markets order.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withEventTime(std::int64_t eventTime) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withEventTime(eventTime));
    }

    // OrderBase methods

    /**
     * Changes event's source and returns the current OTC Markets order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source source of this event.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withSource(const OrderSource &source) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withSource(source));
    }

    /**
     * Changes transactional event flags and returns the current OTC Markets order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withEventFlags(std::int32_t eventFlags) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withEventFlags(eventFlags));
    }

    /**
     * Changes transactional event flags and returns the current OTC Markets order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withEventFlags(eventFlags));
    }

    /**
     * Changes unique per-symbol index of this OTC Markets order and returns it. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index unique per-symbol index of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withIndex(std::int64_t index) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withIndex(index));
    }

    /**
     * Changes time of this OTC Markets order and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTime(std::int64_t time) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTime(time));
    }

    /**
     * Changes microseconds and nanoseconds time part of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTimeNanoPart(timeNanoPart));
    }

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param sequence the sequence.
     * @return The current OTC Markets order.
     * @see OrderBase::getSequence()
     */
    OtcMarketsOrder &withSequence(std::int32_t sequence) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withSequence(sequence));
    }

    /**
     * Changes time of this OTC Markets order and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this OTC Markets order in nanoseconds.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTimeNanos(std::int64_t timeNanos) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTimeNanos(timeNanos));
    }

    /**
     * Changes action of this OTC Markets order and returns it.
     *
     * @param action The side of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withAction(const OrderAction &action) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withAction(action));
    }

    /**
     * Changes time of the last action and returns current OTC Markets order.
     *
     * @param actionTime The last order action time.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withActionTime(std::int64_t actionTime) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withActionTime(actionTime));
    }

    /**
     * Changes order ID.
     * Returns the current OTC Markets order.
     *
     * @param orderId The order ID.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withOrderId(std::int64_t orderId) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withOrderId(orderId));
    }

    /**
     * Changes auxiliary order ID.
     * Returns the current OTC Markets order.
     *
     * @param auxOrderId The auxiliary order ID.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withAuxOrderId(std::int64_t auxOrderId) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withAuxOrderId(auxOrderId));
    }

    /**
     * Changes price of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param price The price of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withPrice(double price) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withPrice(price));
    }

    /**
     * Changes size of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param size The size of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withSize(double size) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withSize(size));
    }

    /**
     * Changes executed size of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param executedSize The executed size of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withExecutedSize(double executedSize) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withExecutedSize(executedSize));
    }

    /**
     * Changes number of individual orders in this aggregate order.
     * Returns the current OTC Markets order.
     *
     * @param count The number of individual orders in this aggregate order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withCount(std::int64_t count) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withCount(count));
    }

    /**
     * Changes trade ID.
     * Returns the current OTC Markets order.
     *
     * @param tradeId The trade ID.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTradeId(std::int64_t tradeId) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTradeId(tradeId));
    }

    /**
     * Changes trade price.
     * Returns the current OTC Markets order.
     *
     * @param tradePrice The trade price.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTradePrice(double tradePrice) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTradePrice(tradePrice));
    }

    /**
     * Changes trade size.
     * Returns the current OTC Markets order.
     *
     * @param tradeSize The trade size.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withTradeSize(double tradeSize) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withTradeSize(tradeSize));
    }

    /**
     * Changes exchange code of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param exchangeCode The exchange code of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withExchangeCode(char exchangeCode) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withExchangeCode(exchangeCode));
    }

    /**
     * Changes exchange code of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param exchangeCode The exchange code of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withExchangeCode(std::int16_t exchangeCode) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withExchangeCode(exchangeCode));
    }

    /**
     * Changes side of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param side The side of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withOrderSide(const Side &side) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withOrderSide(side));
    }

    /**
     * Changes scope of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param scope The scope of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withScope(const Scope &scope) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withScope(scope));
    }

    // Order methods

    /**
     * Changes market maker or other aggregate identifier of this OTC Markets order.
     * Returns the current OTC Markets order.
     *
     * @param marketMaker The market maker or other aggregate identifier of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withMarketMaker(std::string marketMaker) noexcept {
        return dynamic_cast<OtcMarketsOrder &>(Order::withMarketMaker(std::move(marketMaker)));
    }

    // OtcMarketsOrder methods

    /**
     * Returns Quote Access Payment (QAP) of this OTC Markets order.
     * QAP functionality allows participants to dynamically set access fees or rebates,
     * in real-time and on a per-security basis through OTC Dealer or OTC FIX connections.
     * Positive integers (1 to 30) indicate a rebate, and negative integers (-1 to -30) indicate an access fee.
     * 0 indicates no rebate or access fee.
     *
     * @return QAP of this OTC Markets order.
     */
    std::int32_t getQuoteAccessPayment() const noexcept {
        return otcMarketsOrderData_.quoteAccessPayment;
    }

    /**
     * Changes Quote Access Payment (QAP) of this OTC Markets order.
     *
     * @param quoteAccessPayment QAP of this OTC Markets order.
     */
    void setQuoteAccessPayment(std::int32_t quoteAccessPayment) noexcept {
        otcMarketsOrderData_.quoteAccessPayment = quoteAccessPayment;
    }

    /**
     * Changes Quote Access Payment (QAP) and returns the current OTC Markets order.
     *
     * @param quoteAccessPayment QAP of this OTC Markets order.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withQuoteAccessPayment(std::int32_t quoteAccessPayment) noexcept {
        setQuoteAccessPayment(quoteAccessPayment);

        return *this;
    }

    /**
     * Returns whether this event is available for business within the operating hours of the OTC Link system.
     * All quotes will be closed at the start of the trading day and will remain closed until the traders open theirs.
     *
     * @return `true` if this event is available for business within the operating hours of the OTC Link system.
     */
    bool isOpen() const noexcept {
        return dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, OPEN) != 0;
    }

    /**
     * Changes whether this event is available for business within the operating hours of the OTC Link system.
     *
     * @param open `true` if this event is available for business within the operating hours of the OTC Link system.
     */
    void setOpen(bool open) noexcept {
        otcMarketsOrderData_.otcMarketsFlags = open ? dxfcpp::orOp(otcMarketsOrderData_.otcMarketsFlags, OPEN)
                                                    : dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, ~OPEN);
    }

    /**
     * Changes whether this event is available for business within the operating hours of the OTC Link system.
     * Returns the current OTC Markets order.
     *
     * @param open `true` if this event is available for business within the operating hours of the OTC Link system.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withOpen(bool open) noexcept {
        setOpen(open);

        return *this;
    }

    /**
     * Returns whether this event is unsolicited.
     *
     * @return `true` if this event is unsolicited.
     */
    bool isUnsolicited() const noexcept {
        return dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, UNSOLICITED) != 0;
    }

    /**
     * Changes whether this event is unsolicited.
     *
     * @param unsolicited `true` if this event is unsolicited.
     */
    void setUnsolicited(bool unsolicited) noexcept {
        otcMarketsOrderData_.otcMarketsFlags = unsolicited
                                                   ? dxfcpp::orOp(otcMarketsOrderData_.otcMarketsFlags, UNSOLICITED)
                                                   : dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, ~UNSOLICITED);
    }

    /**
     * Changes whether this event is unsolicited.
     * Returns the current OTC Markets order.
     *
     * @param unsolicited `true` if this event is unsolicited.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withUnsolicited(bool unsolicited) noexcept {
        setUnsolicited(unsolicited);

        return *this;
    }

    /**
     * Returns OTC Markets price type of this OTC Markets order events.
     *
     * @return OTC Markets price type of this OTC Markets order events.
     */
    const OtcMarketsPriceType &getOtcMarketsPriceType() const & noexcept {
        return OtcMarketsPriceType::valueOf(
            getBits(otcMarketsOrderData_.otcMarketsFlags, OTC_PRICE_TYPE_MASK, OTC_PRICE_TYPE_SHIFT));
    }

    /**
     * Changes OTC Markets price type of this OTC Markets order events.
     *
     * @param otcPriceType OTC Markets price type of this OTC Markets order events.
     */
    void setOtcMarketsPriceType(const OtcMarketsPriceType &otcPriceType) noexcept {
        otcMarketsOrderData_.otcMarketsFlags = setBits(otcMarketsOrderData_.otcMarketsFlags, OTC_PRICE_TYPE_MASK,
                                                       OTC_PRICE_TYPE_SHIFT, otcPriceType.getCode());
    }

    /**
     * Changes OTC Markets price type of this OTC Markets order events.
     * Returns the current OTC Markets order.
     *
     * @param otcPriceType OTC Markets price type of this OTC Markets order events.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withOtcMarketsPriceType(const OtcMarketsPriceType &otcPriceType) noexcept {
        setOtcMarketsPriceType(otcPriceType);

        return *this;
    }

    /**
     * Returns whether this event should NOT be considered for the inside price.
     *
     * @return `true` if this event should NOT be considered for the inside price.
     */
    bool isSaturated() const noexcept {
        return dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, SATURATED) != 0;
    }

    /**
     * Changes whether this event should NOT be considered for the inside price.
     *
     * @param saturated `true` if this event should NOT be considered for the inside price.
     */
    void setSaturated(bool saturated) noexcept {
        otcMarketsOrderData_.otcMarketsFlags = saturated
                                                   ? dxfcpp::orOp(otcMarketsOrderData_.otcMarketsFlags, SATURATED)
                                                   : dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, ~SATURATED);
    }

    /**
     * Changes whether this event should NOT be considered for the inside price.
     * Returns the current OTC Markets order.
     *
     * @param saturated `true` if this event should NOT be considered for the inside price.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withSaturated(bool saturated) noexcept {
        setSaturated(saturated);

        return *this;
    }

    /**
     * Returns whether this event is in 'AutoEx' mode.
     * If this event is in 'AutoEx' mode then a response to an OTC Link trade message will be immediate.
     *
     * @return `true` if this event is in 'AutoEx' mode.
     */
    bool isAutoExecution() const noexcept {
        return dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, AUTO_EXECUTION) != 0;
    }

    /**
     * Changes whether this event is in 'AutoEx' mode.
     *
     * @param autoExecution `true` if this event is in 'AutoEx' mode.
     */
    void setAutoExecution(bool autoExecution) noexcept {
        otcMarketsOrderData_.otcMarketsFlags =
            autoExecution ? dxfcpp::orOp(otcMarketsOrderData_.otcMarketsFlags, AUTO_EXECUTION)
                          : dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, ~AUTO_EXECUTION);
    }

    /**
     * Changes whether this event is in 'AutoEx' mode.
     * Returns the current OTC Markets order.
     *
     * @param autoExecution `true` if this event is in 'AutoEx' mode.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withAutoExecution(bool autoExecution) noexcept {
        setAutoExecution(autoExecution);

        return *this;
    }

    /**
     * Returns whether this event represents a NMS conditional.
     * This flag indicates the displayed @ref #getSize() "size" is a round lot at least two times greater than the
     * minimum round lot size in the security and a trade message relating to the event cannot be sent or filled for
     * less than the displayed size.
     *
     * @return `true` if this event represents a NMS conditional.
     */
    bool isNmsConditional() const noexcept {
        return dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, NMS_CONDITIONAL) != 0;
    }

    /**
     * Changes whether this event represents a NMS conditional.
     *
     * @param nmsConditional `true` if this event represents a NMS conditional.
     */
    void setNmsConditional(bool nmsConditional) noexcept {
        otcMarketsOrderData_.otcMarketsFlags =
            nmsConditional ? dxfcpp::orOp(otcMarketsOrderData_.otcMarketsFlags, NMS_CONDITIONAL)
                           : dxfcpp::andOp(otcMarketsOrderData_.otcMarketsFlags, ~NMS_CONDITIONAL);
    }

    /**
     * Changes whether this event represents a NMS conditional.
     * Returns the current OTC Markets order.
     *
     * @param nmsConditional `true` if this event represents a NMS conditional.
     * @return The current OTC Markets order.
     */
    OtcMarketsOrder &withNmsConditional(bool nmsConditional) noexcept {
        setNmsConditional(nmsConditional);

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