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
#include "../IndexedEventSource.hpp"
#include "../TimeSeriesEvent.hpp"
#include "MarketEvent.hpp"
#include "Side.hpp"
#include "TimeAndSaleType.hpp"

namespace dxfcpp {

struct EventMapper;
class OptionSale;

/**
 * Time and Sale represents a trade or other market event with price, like market open/close price, etc.
 * Time and Sales are intended to provide information about trades <b>in a continuous time slice</b>
 * (unlike Trade events which are supposed to provide snapshot about the <b>current last</b> trade).
 *
 * <p> Time and Sale events have unique @ref ::getIndex() "index" which can be used for later
 * correction/cancellation processing.
 *
 * Some time and sale sources provide a consistent view of the set of known time and sales
 * for a given time range when used with DXFeedTimeSeriesSubscription}.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for time and sales, thus
 * @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p> Regular subscription via DXFeedSubscription produces a stream of time and
 * sale events as they happen and their @ref ::getEventFlags() "eventFlags" are always zero.
 *
 * Publishing of time and sales events follows the general rules explained in TimeSeriesEvent class
 * documentation.
 *
 * This event is implemented on top of QDS record `TimeAndSale`.
 */
class DXFCPP_EXPORT TimeAndSale final : public MarketEvent, public TimeSeriesEvent {
    friend struct EventMapper;
    friend class OptionSale;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..16   15...8    7    6    5    4    3    2    1    0
     * +--------+--------+----+----+----+----+----+----+----+----+
     * |        |   TTE  |    |  Side   | SL | ETH| VT |  Type   |
     * +--------+--------+----+----+----+----+----+----+----+----+
     */

    // TTE (TradeThroughExempt) values are ASCII chars in [0, 255].
    static constexpr std::uint32_t TTE_MASK = 0xFFU;
    static constexpr std::uint32_t TTE_SHIFT = 8U;

    // SIDE values are taken from Side enum.
    static constexpr std::uint32_t SIDE_MASK = 3U;
    static constexpr std::uint32_t SIDE_SHIFT = 5U;

    static constexpr std::uint32_t SPREAD_LEG = 1U << 4U;
    static constexpr std::uint32_t ETH = 1U << 3U;
    static constexpr std::uint32_t VALID_TICK = 1U << 2U;

    // TYPE values are taken from TimeAndSaleType enum.
    static constexpr std::uint32_t TYPE_MASK = 3U;
    static constexpr std::uint32_t TYPE_SHIFT = 0;

    /**
     * EventFlags property has several significant bits that are packed into an integer in the following way:
     * <br>
     * <pre><tt>
     *    31..7    6    5    4    3    2    1    0
     * +--------+----+----+----+----+----+----+----+
     * |        | SM |    | SS | SE | SB | RE | TX |
     * +--------+----+----+----+----+----+----+----+
     * </tt></pre>
     */

    struct Data {
        std::int32_t eventFlags{};
        std::int64_t index{};
        std::int32_t timeNanoPart{};
        std::int16_t exchangeCode{};
        double price = math::NaN;
        double size = math::NaN;
        double bidPrice = math::NaN;
        double askPrice = math::NaN;
        std::string exchangeSaleConditions{};
        std::int32_t flags{};
        std::string buyer{};
        std::string seller{};
    };

    Data data_{};

    static std::shared_ptr<TimeAndSale> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new time and sale event with default values.
    TimeAndSale() noexcept = default;

    /**
     * Creates new time and sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TimeAndSale(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

    ///
    const IndexedEventSource &getSource() const & override { return IndexedEventSource::DEFAULT; }

    ///
    EventFlagsMask getEventFlags() const override { return EventFlagsMask(data_.eventFlags); }

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) override {
        data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    /**
     * Returns unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence".
     * Changing either time or sequence changes event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const override { return data_.index; }

    /**
     * Changes unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence" and
     * invocation of this method changes time and sequence.
     * <b>Do not use this method directly.</b>
     * Change @ref ::setTime() "time" and/or @ref ::setSequence() "sequence".
     *
     * @param index the event index.
     * @see ::getIndex()
     */
    void setIndex(std::int64_t index) override { data_.index = index; }

    /**
     * Returns timestamp of the event in milliseconds.
     *
     * @return timestamp of the event in milliseconds
     */
    std::int64_t getTime() const override {
        return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes timestamp of the event in milliseconds.
     *
     * @param time timestamp of the event in milliseconds.
     * @see ::getTime()
     */
    void setTime(std::int64_t time) {
        data_.index = orOp(orOp(sal(time_util::getSecondsFromTime(time), SECONDS_SHIFT),
                                sal(time_util::getMillisFromTime(time), MILLISECONDS_SHIFT)),
                           getSequence());
    }

    /**
     * Returns time of the original event in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the original event in nanoseconds.
     */
    std::int64_t getTimeNanos() const {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Changes time of the original event.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the original event in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) {
        setTime(time_nanos_util::getMillisFromNanos(timeNanos));
        data_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
    }

    /**
     * Changes microseconds and nanoseconds time part of the original event.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the original event.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) { data_.timeNanoPart = timeNanoPart; }

    /**
     * Returns microseconds and nanoseconds time part of the original event.
     *
     * @return microseconds and nanoseconds time part of the original event.
     */
    std::int32_t getTimeNanoPart() const { return data_.timeNanoPart; }

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const { return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE)); }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @see ::getSequence()
     */
    void setSequence(int sequence) {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns exchange code of this time and sale event.
     *
     * @return exchange code of this time and sale event.
     */
    std::int16_t getExchangeCode() const { return data_.exchangeCode; }

    /**
     * Changes exchange code of this time and sale event.
     *
     * @param exchangeCode exchange code of this time and sale event.
     */
    void setExchangeCode(char exchangeCode);

    /**
     * Changes exchange code of this time and sale event.
     *
     * @param exchangeCode exchange code of this time and sale event.
     */
    void setExchangeCode(std::int16_t exchangeCode) { data_.exchangeCode = exchangeCode; }

    /**
     * Returns price of this time and sale event.
     *
     * @return price of this time and sale event.
     */
    double getPrice() const { return data_.price; }

    /**
     * Changes price of this time and sale event.
     *
     * @param price price of this time and sale event.
     */
    void setPrice(double price) { data_.price = price; }

    /**
     * Returns size of this time and sale event.
     *
     * @return size of this time and sale event.
     */
    double getSize() const { return data_.size; }

    /**
     * Changes size of this time and sale event.
     *
     * @param size size of this time and sale event.
     */
    void setSize(double size) { data_.size = size; }

    /**
     * Returns the current bid price on the market when this time and sale event had occurred.
     *
     * @return the current bid price on the market when this time and sale event had occurred.
     */
    double getBidPrice() const { return data_.bidPrice; }

    /**
     * Changes the current bid price on the market when this time and sale event had occurred.
     *
     * @param bidPrice the current bid price on the market when this time and sale event had occurred.
     */
    void setBidPrice(double bidPrice) { data_.bidPrice = bidPrice; }

    /**
     * Returns the current ask price on the market when this time and sale event had occurred.
     *
     * @return the current ask price on the market when this time and sale event had occurred.
     */
    double getAskPrice() const { return data_.askPrice; }

    /**
     * Changes price of this time and sale event.the current ask price on the market when this time and sale event had
     * occurred.
     *
     * @param askPrice the current ask price on the market when this time and sale event had occurred.
     */
    void setAskPrice(double askPrice) { data_.askPrice = askPrice; }

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions.
     */
    const std::string &getExchangeSaleConditions() const & { return data_.exchangeSaleConditions; }

    /**
     * Changes sale conditions provided for this event by data feed.
     *
     * @param exchangeSaleConditions sale conditions.
     */
    void setExchangeSaleConditions(std::string exchangeSaleConditions) {
        data_.exchangeSaleConditions = std::move(exchangeSaleConditions);
    }

    /**
     * Returns TradeThroughExempt flag of this time and sale event.
     *
     * @return TradeThroughExempt flag of this time and sale event.
     */
    char getTradeThroughExempt() const {
        return static_cast<char>(static_cast<unsigned char>(getBits(data_.flags, TTE_MASK, TTE_SHIFT)));
    }

    /**
     * Changes TradeThroughExempt flag of this time and sale event.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this time and sale event.
     */
    void setTradeThroughExempt(char tradeThroughExempt) {
        // TODO: error handling //util::checkChar(tradeThroughExempt, TTE_MASK, "tradeThroughExempt");

        data_.flags = setBits(data_.flags, TTE_MASK, TTE_SHIFT, tradeThroughExempt);
    }

    /**
     * Returns aggressor side of this time and sale event.
     *
     * @return aggressor side of this time and sale event.
     */
    const Side &getAggressorSide() const & { return Side::valueOf(getBits(data_.flags, SIDE_MASK, SIDE_SHIFT)); }

    /**
     * Changes aggressor side of this time and sale event.
     *
     * @param side aggressor side of this time and sale event.
     */
    void setAggressorSide(const Side &side) {
        data_.flags = setBits(data_.flags, SIDE_MASK, SIDE_SHIFT, side.getCode());
    }

    /**
     * Returns whether this event represents a spread leg.
     *
     * @return `true` if this event represents a spread leg.
     */
    bool isSpreadLeg() const { return andOp(data_.flags, SPREAD_LEG) != 0; }

    /**
     * Changes whether this event represents a spread leg.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     */
    void setSpreadLeg(bool spreadLeg) {
        data_.flags = spreadLeg ? orOp(data_.flags, SPREAD_LEG) : andOp(data_.flags, ~SPREAD_LEG);
    }

    /**
     * Returns whether this event represents an extended trading hours sale.
     *
     * @return `true` if this event represents an extended trading hours sale.
     */
    bool isExtendedTradingHours() const { return andOp(data_.flags, ETH) != 0; }

    /**
     * Changes whether this event represents an extended trading hours sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     */
    void setExtendedTradingHours(bool extendedTradingHours) {
        data_.flags = extendedTradingHours ? orOp(data_.flags, ETH) : andOp(data_.flags, ~ETH);
    }

    /**
     * Returns whether this event represents a valid intraday tick.
     * Note, that a correction for a previously distributed valid tick represents a new valid tick itself,
     * but a cancellation of a previous valid tick does not.
     *
     * @return `true` if this event represents a valid intraday tick.
     */
    bool isValidTick() const { return andOp(data_.flags, VALID_TICK) != 0; }

    /**
     * Changes whether this event represents a valid intraday tick.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     */
    void setValidTick(bool validTick) {
        data_.flags = validTick ? orOp(data_.flags, VALID_TICK) : andOp(data_.flags, ~VALID_TICK);
    }

    /**
     * Returns type of this time and sale event.
     *
     * @return type of this time and sale event.
     */
    const TimeAndSaleType &getType() const & {
        return TimeAndSaleType::valueOf(getBits(data_.flags, TYPE_MASK, TYPE_SHIFT));
    }

    /**
     * Changes type of this time and sale event.
     *
     * @param type type of this time and sale event.
     */
    void setType(const TimeAndSaleType &type) {
        data_.flags = setBits(data_.flags, TYPE_MASK, TYPE_SHIFT, type.getCode());
    }

    /**
     * Returns whether this is a new event (not cancellation or correction).
     * It is `true` for newly created time and sale event.
     *
     * @return `true` if this is a new event (not cancellation or correction).
     */
    bool isNew() const { return getType() == TimeAndSaleType::NEW; }

    /**
     * Returns whether this is a correction of a previous event.
     * It is `false` for newly created time and sale event.
     *
     * @return `true` if this is a correction of a previous event
     */
    bool isCorrection() const { return getType() == TimeAndSaleType::CORRECTION; }

    /**
     * Returns whether this is a cancellation of a previous event.
     * It is `false` for newly created time and sale event.
     * @return `true` if this is a cancellation of a previous event
     */
    bool isCancel() const { return getType() == TimeAndSaleType::CANCEL; }

    /**
     * Returns buyer of this time and sale event.
     *
     * @return buyer of this time and sale event.
     */
    const std::string &getBuyer() const & { return data_.buyer; }

    /**
     * Changes buyer of this time and sale event.
     *
     * @param buyer buyer of this time and sale event.
     */
    void setBuyer(std::string buyer) { data_.buyer = std::move(buyer); }

    /**
     * Returns seller of this time and sale event.
     *
     * @return seller of this time and sale event.
     */
    const std::string &getSeller() const & { return data_.seller; }

    /**
     * Changes seller of this time and sale event.
     *
     * @param seller seller of this time and sale event.
     */
    void setSeller(std::string seller) { data_.seller = std::move(seller); }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp