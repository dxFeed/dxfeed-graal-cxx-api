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
#include "../IndexedEvent.hpp"
#include "../IndexedEventSource.hpp"
#include "MarketEvent.hpp"
#include "Side.hpp"
#include "TimeAndSale.hpp"
#include "TimeAndSaleType.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Option Sale event represents a trade or another market event with the price
 * (for example, market open/close price, etc.) for each option symbol listed under the specified Underlying.
 * Option Sales are intended to provide information about option trades <b>in a continuous time slice</b> with
 * the additional metrics, like Option Volatility, Option Delta, and Underlying Price.
 *
 * <p>Option Sale events have unique @ref ::getIndex() "index" which can be used for later
 * correction/cancellation processing.
 *
 * Option sale data source provides a consistent view of the set of known option sales.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for option sale events, thus
 * @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * This event is implemented on top of QDS records `OptionSale`.
 */
class DXFCPP_EXPORT OptionSale final : public MarketEvent, public IndexedEvent {
    friend struct EventMapper;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

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
        std::int64_t timeSequence{};
        std::int32_t timeNanoPart{};
        std::int16_t exchangeCode{};
        double price = math::NaN;
        double size = math::NaN;
        double bidPrice = math::NaN;
        double askPrice = math::NaN;
        std::string exchangeSaleConditions{};
        std::int32_t flags{};
        double underlyingPrice = math::NaN;
        double volatility = math::NaN;
        double delta = math::NaN;
        std::string optionSymbol;
    };

    Data data_{};

    static std::shared_ptr<OptionSale> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new option sale event with default values.
    OptionSale() noexcept = default;

    /**
     * Creates new option sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OptionSale(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

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
     * Returns time and sequence of this event packaged into single long value.
     *
     * @return time and sequence of this event.
     */
    std::int64_t getTimeSequence() const { return data_.timeSequence; }

    /**
     * Changes time and sequence of this event.
     * <b>Do not use this method directly.</b>
     * Change @ref ::setTime() "time" and/or @ref ::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) { data_.timeSequence = timeSequence; }

    /**
     * Returns time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this event.
     */
    std::int64_t getTime() const {
        return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
               andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this event.
     */
    void setTime(std::int64_t time) {
        data_.timeSequence = orOp(
            sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
            orOp(sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_MASK), getSequence()));
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
     * Returns exchange code of this option sale event.
     *
     * @return exchange code of this option sale event.
     */
    std::int16_t getExchangeCode() const { return data_.exchangeCode; }

    /**
     * Changes exchange code of this option sale event.
     *
     * @param exchangeCode exchange code of this option sale event.
     */
    void setExchangeCode(char exchangeCode);

    /**
     * Changes exchange code of this option sale event.
     *
     * @param exchangeCode exchange code of this option sale event.
     */
    void setExchangeCode(std::int16_t exchangeCode) { data_.exchangeCode = exchangeCode; }

    /**
     * Returns price of this option sale event.
     *
     * @return price of this option sale event.
     */
    double getPrice() const { return data_.price; }

    /**
     * Changes price of this option sale event.
     *
     * @param price price of this option sale event.
     */
    void setPrice(double price) { data_.price = price; }

    /**
     * Returns size of this option sale event.
     *
     * @return size of this option sale event.
     */
    double getSize() const { return data_.size; }

    /**
     * Changes size of this option sale event.
     *
     * @param size size of this option sale event.
     */
    void setSize(double size) { data_.size = size; }

    /**
     * Returns the current bid price on the market when this option sale event had occurred.
     *
     * @return the current bid price on the market when this option sale event had occurred.
     */
    double getBidPrice() const { return data_.bidPrice; }

    /**
     * Changes the current bid price on the market when this option sale event had occurred.
     *
     * @param bidPrice the current bid price on the market when this option sale event had occurred.
     */
    void setBidPrice(double bidPrice) { data_.bidPrice = bidPrice; }

    /**
     * Returns the current ask price on the market when this option sale event had occurred.
     *
     * @return the current ask price on the market when this option sale event had occurred.
     */
    double getAskPrice() const { return data_.askPrice; }

    /**
     * Changes price of this time and sale event.the current ask price on the market when this option sale event had
     * occurred.
     *
     * @param askPrice the current ask price on the market when this option sale event had occurred.
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
     * Returns TradeThroughExempt flag of this option sale event.
     *
     * @return TradeThroughExempt flag of this option sale event.
     */
    char getTradeThroughExempt() const {
        return static_cast<char>(
            static_cast<unsigned char>(getBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT)));
    }

    /**
     * Changes TradeThroughExempt flag of this option sale event.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this option sale event.
     */
    void setTradeThroughExempt(char tradeThroughExempt) {
        // TODO: error handling //util::checkChar(tradeThroughExempt, TimeAndSale::TTE_MASK, "tradeThroughExempt");

        data_.flags = setBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT, tradeThroughExempt);
    }

    /**
     * Returns aggressor side of this option sale event.
     *
     * @return aggressor side of this option sale event.
     */
    const Side &getAggressorSide() const & {
        return Side::valueOf(getBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT));
    }

    /**
     * Changes aggressor side of this option sale event.
     *
     * @param side aggressor side of this option sale event.
     */
    void setAggressorSide(const Side &side) {
        data_.flags = setBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT, side.getCode());
    }

    /**
     * Returns whether this event represents a spread leg.
     *
     * @return `true` if this event represents a spread leg.
     */
    bool isSpreadLeg() const { return andOp(data_.flags, TimeAndSale::SPREAD_LEG) != 0; }

    /**
     * Changes whether this event represents a spread leg.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     */
    void setSpreadLeg(bool spreadLeg) {
        data_.flags =
            spreadLeg ? orOp(data_.flags, TimeAndSale::SPREAD_LEG) : andOp(data_.flags, ~TimeAndSale::SPREAD_LEG);
    }

    /**
     * Returns whether this event represents an extended trading hours sale.
     *
     * @return `true` if this event represents an extended trading hours sale.
     */
    bool isExtendedTradingHours() const { return andOp(data_.flags, TimeAndSale::ETH) != 0; }

    /**
     * Changes whether this event represents an extended trading hours sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     */
    void setExtendedTradingHours(bool extendedTradingHours) {
        data_.flags =
            extendedTradingHours ? orOp(data_.flags, TimeAndSale::ETH) : andOp(data_.flags, ~TimeAndSale::ETH);
    }

    /**
     * Returns whether this event represents a valid intraday tick.
     * Note, that a correction for a previously distributed valid tick represents a new valid tick itself,
     * but a cancellation of a previous valid tick does not.
     *
     * @return `true` if this event represents a valid intraday tick.
     */
    bool isValidTick() const { return andOp(data_.flags, TimeAndSale::VALID_TICK) != 0; }

    /**
     * Changes whether this event represents a valid intraday tick.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     */
    void setValidTick(bool validTick) {
        data_.flags =
            validTick ? orOp(data_.flags, TimeAndSale::VALID_TICK) : andOp(data_.flags, ~TimeAndSale::VALID_TICK);
    }

    /**
     * Returns type of this option sale event.
     *
     * @return type of this option sale event.
     */
    const TimeAndSaleType &getType() const & {
        return TimeAndSaleType::valueOf(getBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT));
    }

    /**
     * Changes type of this option sale event.
     *
     * @param type type of this option sale event.
     */
    void setType(const TimeAndSaleType &type) {
        data_.flags = setBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT, type.getCode());
    }

    /**
     * Returns whether this is a new event (not cancellation or correction).
     * It is `true` for newly created option sale event.
     *
     * @return `true` if this is a new event (not cancellation or correction).
     */
    bool isNew() const { return getType() == TimeAndSaleType::NEW; }

    /**
     * Returns whether this is a correction of a previous event.
     * It is `false` for newly created option sale event.
     *
     * @return `true` if this is a correction of a previous event
     */
    bool isCorrection() const { return getType() == TimeAndSaleType::CORRECTION; }

    /**
     * Returns whether this is a cancellation of a previous event.
     * It is `false` for newly created option sale event.
     * @return `true` if this is a cancellation of a previous event
     */
    bool isCancel() const { return getType() == TimeAndSaleType::CANCEL; }

    /**
     * Returns underlying price at the time of this option sale event.
     *
     * @return underlying price at the time of this option sale event.
     */
    double getUnderlyingPrice() const { return data_.underlyingPrice; }

    /**
     * Changes underlying price at the time of this option sale event.
     *
     * @param underlyingPrice underlying price at the time of this option sale event.
     */
    void setUnderlyingPrice(double underlyingPrice) { data_.underlyingPrice = underlyingPrice; }

    /**
     * Returns Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @return Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    double getVolatility() const { return data_.volatility; }

    /**
     * Changes Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @param volatility Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    void setVolatility(double volatility) { data_.volatility = volatility; }

    /**
     * Return option delta at the time of this option sale event.
     * Delta is the first derivative of an option price by an underlying price.
     *
     * @return option delta at the time of this option sale event.
     */
    double getDelta() const { return data_.delta; }

    /**
     * Changes option delta at the time of this option sale event.
     *
     * @param delta option delta at the time of this option sale event.
     */
    void setDelta(double delta) { data_.delta = delta; }

    /**
     * Returns option symbol of this event.
     *
     * @return option symbol of this event.
     */
    const std::string &getOptionSymbol() const & { return data_.optionSymbol; }

    /**
     * Changes option symbol of this event.
     *
     * @param optionSymbol option symbol of this event.
     */
    void setOptionSymbol(std::string optionSymbol) { data_.optionSymbol = std::move(optionSymbol); }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp