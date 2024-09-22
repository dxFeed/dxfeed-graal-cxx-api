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
#include "../IndexedEvent.hpp"
#include "../IndexedEventSource.hpp"
#include "MarketEvent.hpp"
#include "OrderAction.hpp"
#include "OrderSource.hpp"
#include "Scope.hpp"
#include "Side.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Base class for common fields of Order, AnalyticOrder and SpreadOrder events.
 * Order events represent a snapshot for a full available market depth for a symbol.
 * The collection of order events of a symbol represents the most recent information that is
 * available about orders on the market at any given moment of time.
 *
 * <p>Order event represents market depth for a <b>specific symbol</b>.
 *
 * <p>AnalyticOrder event represents market depth for a <b>specific symbol</b> extended with an analytic
 * information, for example, whether particular order represent an iceberg or not.
 *
 * <p>SpreadOrder event represents market depth for
 *    <b>all spreads on a given underlying symbol</b>.
 *
 * <p> Order events arrive from
 * multiple sources for the same market symbol and are distinguished by their
 * @ref OrderBase::getIndex "index". Index is a unique per symbol identifier of the event.
 * It is unique across all the sources of depth information for the symbol.
 * The event with @ref OrderBase::getSize() "size" either `0` or `NaN` is a signal to remove previously received order
 * for the corresponding index.
 * The method @ref OrderBase::hasSize() "hasSize" is a convenient method to test for size presence.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some order event sources provide a consistent view of the price-level or detailed order book. Their updates
 * may incorporate multiple changes to price levels or to individual orders that have to be processed at the same time.
 * The corresponding information is carried in @ref OrderBase::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 *
 * <p> The event @ref OrderBase::getSource() "source" identifier for an order is a part of the unique event @ref
 * OrderBase::getIndex() "index". It occupies highest bits of the @ref OrderBase::getIndex() "index" (index is
 * not-negative). The lowest bits of
 * @ref OrderBase::getIndex() "index" contain source-specific event index which is always zero in
 * an event that is marked with EventFlag::SNAPSHOT_END bit in @ref OrderBase::getEventFlags() "eventFlags".
 *
 * <p> Note that for an order with EventFlag::REMOVE_EVENT bit in @ref OrderBase::getEventFlags() "eventFlags"
 * it is always the case that @ref #getSize() "size" is either `0` or `NaN`,
 * so no additional logic to process this bit is required for orders.
 * Transactions and snapshots may include orders with @ref OrderBase::getSize() "size" of `0`.
 * The filtering that distinguishes those events as removals of orders shall be performed after
 * all transactions and snapshot processing.
 *
 * <p> Some aggregated feeds (like CME market depth) are mapped into two distinct source ids (one for
 * buy orders and one for sell orders), but updates and transactions may span both. It is important to keep a
 * separate track of transactional state for each source id, but, at the same time, when
 * DXFeedEventListener::eventsReceived() method is called for a list
 * of events, the order book shall be considered complete and consistent only when all events from the given
 * list are processed.
 */
class DXFCPP_EXPORT OrderBase : public MarketEvent, public IndexedEvent {
    friend struct EventMapper;

  protected:
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    static constexpr std::uint32_t SPECIAL_SOURCE_ID_SHIFT = 48U;
    static constexpr std::uint32_t NONSPECIAL_SOURCE_ID_SHIFT = 32U;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..15   14..11    10..4    3    2    1    0
     * +--------+--------+--------+----+----+----+----+
     * |        | Action |Exchange|  Side   |  Scope  |
     * +--------+--------+--------+----+----+----+----+
     */

    // ACTION values are taken from OrderAction enum.
    static constexpr std::uint32_t ACTION_MASK = 0x0FU;
    static constexpr std::uint32_t ACTION_SHIFT = 11U;

    // EXCHANGE values are ASCII chars in [0, 127].
    static constexpr std::uint32_t EXCHANGE_MASK = 0x7FU;
    static constexpr std::uint32_t EXCHANGE_SHIFT = 4U;

    // SIDE values are taken from Side enum.
    static constexpr std::uint32_t SIDE_MASK = 3U;
    static constexpr std::uint32_t SIDE_SHIFT = 2U;

    // SCOPE values are taken from Scope enum.
    static constexpr std::uint32_t SCOPE_MASK = 3U;
    static constexpr std::uint32_t SCOPE_SHIFT = 0;

    /*
     * Index field contains source identifier, optional exchange code and low-end index (virtual id or MMID).
     * Index field has 2 formats depending on whether source is "special" (see OrderSource.isSpecialSourceId()).
     * Note: both formats are IMPLEMENTATION DETAILS, they are subject to change without notice.
     *   63..48   47..32   31..16   15..0
     * +--------+--------+--------+--------+
     * | Source |Exchange|      Index      |  <- "special" order sources (non-printable id with exchange)
     * +--------+--------+--------+--------+
     *   63..48   47..32   31..16   15..0
     * +--------+--------+--------+--------+
     * |     Source      |      Index      |  <- generic order sources (alphanumeric id without exchange)
     * +--------+--------+--------+--------+
     * Note: when modifying formats track usages of getIndex/setIndex, getSource/setSource and isSpecialSourceId
     * methods in order to find and modify all code dependent on current formats.
     */

    /*
     * EventFlags property has several significant bits that are packed into an integer in the following way:
     *    31..7    6    5    4    3    2    1    0
     * +---------+----+----+----+----+----+----+----+
     * |         | SM |    | SS | SE | SB | RE | TX |
     * +---------+----+----+----+----+----+----+----+
     */

    struct OrderBaseData {
        std::int32_t eventFlags{};
        std::int64_t index{};
        std::int64_t timeSequence{};
        std::int32_t timeNanoPart{};
        std::int64_t actionTime{};
        std::int64_t orderId{};
        std::int64_t auxOrderId{};
        double price = math::NaN;
        double size = math::NaN;
        double executedSize = math::NaN;
        std::int64_t count{};
        std::int32_t flags{};
        std::int64_t tradeId{};
        double tradePrice = math::NaN;
        double tradeSize = math::NaN;
    };

    OrderBaseData orderBaseData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /**
     * Maximum allowed sequence value.
     *
     * @see OrderBase::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    ///
    void assign(std::shared_ptr<EventType> event) override;

    /// Creates new order event with default values.
    OrderBase() noexcept = default;

    /**
     * Creates new order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OrderBase(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

    /**
     * Returns source of this event.
     * The source is stored in the highest bits of the @ref getIndex() "index" of this event.
     *
     * @return source of this event.
     */
    const OrderSource &getSource() const & noexcept override {
        auto sourceId = static_cast<std::int32_t>(sar(orderBaseData_.index, SPECIAL_SOURCE_ID_SHIFT));

        if (!OrderSource::isSpecialSourceId(sourceId)) {
            sourceId = static_cast<std::int32_t>(sar(orderBaseData_.index, NONSPECIAL_SOURCE_ID_SHIFT));
        }

        return OrderSource::valueOf(sourceId);
    }

    /**
     * Changes source of this event.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source source of this event.
     */
    void setSource(const OrderSource &source) noexcept {
        auto shift = OrderSource::isSpecialSourceId(source.id()) ? SPECIAL_SOURCE_ID_SHIFT : NONSPECIAL_SOURCE_ID_SHIFT;
        std::int64_t mask = OrderSource::isSpecialSourceId(
                                static_cast<std::int32_t>(sar(orderBaseData_.index, SPECIAL_SOURCE_ID_SHIFT)))
                                ? ~sal(std::int64_t{-1}, SPECIAL_SOURCE_ID_SHIFT)
                                : ~sal(std::int64_t{-1}, NONSPECIAL_SOURCE_ID_SHIFT);
        orderBaseData_.index =
            andOp(sal(static_cast<std::int64_t>(source.id()), shift), andOp(orderBaseData_.index, mask));
    }

    ///
    std::int32_t getEventFlags() const noexcept override {
        return orderBaseData_.eventFlags;
    }

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override {
        return EventFlagsMask(orderBaseData_.eventFlags);
    }

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override {
        orderBaseData_.eventFlags = eventFlags;
    }

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override {
        orderBaseData_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    /**
     * Changes unique per-symbol index of this order. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index unique per-symbol index of this order.
     * @throws InvalidArgumentException
     */
    void setIndex(std::int64_t index) override {
        if (index < 0) {
            throw InvalidArgumentException("Negative index: " + std::to_string(index));
        }

        orderBaseData_.index = index;
    }

    /**
     * Returns unique per-symbol index of this order. Index is non-negative.
     *
     * @return unique per-symbol index of this order.
     */
    std::int64_t getIndex() const noexcept override {
        return orderBaseData_.index;
    }

    /**
     * Returns time and sequence of this order packaged into single long value.
     * This method is intended for efficient order time priority comparison.
     *
     * @return time and sequence of this order.
     */
    std::int64_t getTimeSequence() const noexcept {
        return orderBaseData_.timeSequence;
    }

    /**
     * Changes time and sequence of this order.
     * @warning <b>Do not use this method directly.</b>
     * Change @ref OrderBase::setTime() "time" and/or @ref OrderBase::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see OrderBase::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept {
        orderBaseData_.timeSequence = timeSequence;
    }

    /**
     * Returns time of this order.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this order.
     */
    std::int64_t getTime() const noexcept {
        return sar(orderBaseData_.timeSequence, SECONDS_SHIFT) * 1000 +
               andOp(sar(orderBaseData_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of this order.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this order.
     */
    void setTime(std::int64_t time) noexcept {
        orderBaseData_.timeSequence =
            orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                      sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                 getSequence());
    }

    /**
     * Changes microseconds and nanoseconds time part of this order.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this order.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        orderBaseData_.timeNanoPart = timeNanoPart;
    }

    /**
     * Returns microseconds and nanoseconds time part of this order.
     *
     * @return microseconds and nanoseconds time part of this order.
     */
    std::int32_t getTimeNanoPart() const noexcept {
        return orderBaseData_.timeNanoPart;
    }

    /**
     * Returns sequence number of this order to distinguish orders that have the same @ref OrderBase::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to OrderBase::MAX_SEQUENCE.
     *
     * @return sequence of this order.
     */
    std::int32_t getSequence() const noexcept {
        return static_cast<std::int32_t>(andOp(orderBaseData_.timeSequence, MAX_SEQUENCE));
    }

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this order.
     *
     * @param sequence the sequence.
     *
     * @see OrderBase::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence) {
        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
        }

        orderBaseData_.timeSequence = orOp(andOp(orderBaseData_.timeSequence, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns time of this order in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this order in nanoseconds
     */
    std::int64_t getTimeNanos() const noexcept {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), orderBaseData_.timeNanoPart);
    }

    /**
     * Changes time of this order.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of this order in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) noexcept {
        setTime(time_nanos_util::getMillisFromNanos(timeNanos));
        orderBaseData_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
    }

    /**
     * Returns order action if available, otherwise - OrderAction::UNDEFINED.
     * <p>This field is a part of the FOB support.
     *
     * @return order action or OrderAction::UNDEFINED.
     */
    const OrderAction &getAction() const & noexcept {
        return OrderAction::valueOf(getBits(orderBaseData_.flags, ACTION_MASK, ACTION_SHIFT));
    }

    /**
     * Changes action of this order.
     *
     * @param action The action of this order.
     */
    void setAction(const OrderAction &action) noexcept {
        orderBaseData_.flags = setBits(orderBaseData_.flags, ACTION_MASK, ACTION_SHIFT, action.getCode());
    }

    /**
     * Returns time of the last @ref getAction() "action".
     * <p>This field is a part of the FOB support.
     *
     * @return time of the last order action.
     */
    std::int64_t getActionTime() const noexcept {
        return orderBaseData_.actionTime;
    }

    /**
     * Changes time of the last action
     *
     * @param actionTime last order action time.
     */
    void setActionTime(std::int64_t actionTime) noexcept {
        orderBaseData_.actionTime = actionTime;
    }

    /**
     * Returns order ID if available. Some actions (OrderAction::TRADE, OrderAction::BUST) have no order ID since they
     * are not related to any order in Order book.
     * <p>This field is a part of the FOB support.
     *
     * @return order ID or 0 if not available.
     */
    std::int64_t getOrderId() const noexcept {
        return orderBaseData_.orderId;
    }

    /**
     * Changes order ID.
     *
     * @param orderId order ID.
     */
    void setOrderId(std::int64_t orderId) noexcept {
        orderBaseData_.orderId = orderId;
    }

    /**
     * Returns auxiliary order ID if available:
     * <ul>
     * <li>in OrderAction::NEW - ID of the order replaced by this new order</li>
     * <li>in OrderAction::DELETE - ID of the order that replaces this deleted order</li>
     * <li>in OrderAction::PARTIAL - ID of the aggressor order</li>
     * <li>in OrderAction::EXECUTE - ID of the aggressor order</li>
     * </ul>
     * <p>This field is a part of the FOB support.
     *
     * @return auxiliary order ID or 0 if not available.
     */
    std::int64_t getAuxOrderId() const noexcept {
        return orderBaseData_.auxOrderId;
    }

    /**
     * Changes auxiliary order ID.
     *
     * @param auxOrderId auxiliary order ID.
     */
    void setAuxOrderId(std::int64_t auxOrderId) noexcept {
        orderBaseData_.auxOrderId = auxOrderId;
    }

    /**
     * Returns price of this order.
     *
     * @return price of this order.
     */
    double getPrice() const noexcept {
        return orderBaseData_.price;
    }

    /**
     * Changes price of this order.
     *
     * @param price price of this order.
     */
    void setPrice(double price) noexcept {
        orderBaseData_.price = price;
    }

    /**
     * Returns size of this order.
     *
     * @return size of this order.
     */
    double getSize() const noexcept {
        return orderBaseData_.size;
    }

    /**
     * Changes size of this order.
     *
     * @param size size of this order.
     */
    void setSize(double size) noexcept {
        orderBaseData_.size = size;
    }

    /**
     * Returns `true` if this order has some size (sizeAsDouble is neither `0` nor `NaN`).
     *
     * @return `true` if this order has some size (sizeAsDouble is neither `0` nor `NaN`).
     */
    bool hasSize() const noexcept {
        return orderBaseData_.size != 0 && !std::isnan(orderBaseData_.size);
    }

    /**
     * Returns executed size of this order.
     *
     * @return executed size of this order.
     */
    double getExecutedSize() const noexcept {
        return orderBaseData_.executedSize;
    }

    /**
     * Changes executed size of this order.
     *
     * @param executedSize executed size of this order.
     */
    void setExecutedSize(double executedSize) noexcept {
        orderBaseData_.executedSize = executedSize;
    }

    /**
     * Returns number of individual orders in this aggregate order.
     *
     * @return number of individual orders in this aggregate order.
     */
    std::int64_t getCount() const noexcept {
        return orderBaseData_.count;
    }

    /**
     * Changes number of individual orders in this aggregate order.
     *
     * @param count number of individual orders in this aggregate order.
     */
    void setCount(std::int64_t count) noexcept {
        orderBaseData_.count = count;
    }

    /**
     * Returns trade (order execution) ID for events containing trade-related action.
     * <p>This field is a part of the FOB support.
     *
     * @return trade ID or 0 if not available.
     */
    std::int64_t getTradeId() const noexcept {
        return orderBaseData_.tradeId;
    }

    /**
     * Changes trade ID.
     *
     * @param tradeId trade ID.
     */
    void setTradeId(std::int64_t tradeId) noexcept {
        orderBaseData_.tradeId = tradeId;
    }

    /**
     * Returns trade price for events containing trade-related action.
     * <p>This field is a part of the FOB support.
     *
     * @return trade price of this action.
     */
    double getTradePrice() const noexcept {
        return orderBaseData_.tradePrice;
    }

    /**
     * Changes trade price.
     *
     * @param tradePrice trade price.
     */
    void setTradePrice(double tradePrice) noexcept {
        orderBaseData_.tradePrice = tradePrice;
    }

    /**
     * Returns trade size for events containing trade-related action.
     * <p>This field is a part of the FOB support.
     *
     * @return trade size.
     */
    double getTradeSize() const noexcept {
        return orderBaseData_.tradeSize;
    }

    /**
     * Changes trade size.
     *
     * @param tradeSize trade size.
     */
    void setTradeSize(double tradeSize) noexcept {
        orderBaseData_.tradeSize = tradeSize;
    }

    /**
     * Returns exchange code of this order.
     *
     * @return exchange code of this order.
     */
    std::int16_t getExchangeCode() const noexcept {
        return utf8to16(static_cast<char>(
            static_cast<unsigned char>(getBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT))));
    }

    /**
     * Returns exchange code of this order as UTF8 string.
     *
     * @return exchange code of this order as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept {
        // TODO: cache [EN-8231]

        return std::string( // NOLINT(*-return-braced-init-list)
            1ULL, static_cast<char>(
                      static_cast<unsigned char>(getBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT))));
    }

    /**
     * Changes exchange code of this order.
     *
     * @param exchangeCode exchange code of this order.
     */
    void setExchangeCode(char exchangeCode) {
        util::checkChar(exchangeCode, EXCHANGE_MASK, "exchangeCode");

        orderBaseData_.flags =
            setBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT, static_cast<unsigned char>(exchangeCode));
    }

    /**
     * Changes exchange code of this order.
     *
     * @param exchangeCode exchange code of this order.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept {
        setExchangeCode(utf16to8(exchangeCode));
    }

    /**
     * Returns side of this order.
     *
     * @return side of this order.
     */
    const Side &getOrderSide() const & noexcept {
        return Side::valueOf(getBits(orderBaseData_.flags, SIDE_MASK, SIDE_SHIFT));
    }

    /**
     * Changes side of this order.
     *
     * @param side side of this order.
     */
    void setOrderSide(const Side &side) noexcept {
        orderBaseData_.flags = setBits(orderBaseData_.flags, SIDE_MASK, SIDE_SHIFT, side.getCode());
    }

    /**
     * Returns scope of this order.
     *
     * @return scope of this order.
     */
    const Scope &getScope() const & noexcept {
        return Scope::valueOf(getBits(orderBaseData_.flags, SCOPE_MASK, SCOPE_SHIFT));
    }

    /**
     * Changes scope of this order.
     *
     * @param scope scope of this order.
     */
    void setScope(const Scope &scope) noexcept {
        orderBaseData_.flags = setBits(orderBaseData_.flags, SCOPE_MASK, SCOPE_SHIFT, scope.getCode());
    }

    /**
     * Returns string representation of this order event's fields.
     *
     * @return string representation of this order event's fields.
     */
    std::string baseFieldsToString() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()