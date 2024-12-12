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
#include "../IndexedEvent.hpp"
#include "../IndexedEventSource.hpp"
#include "MarketEvent.hpp"
#include "Side.hpp"
#include "TimeAndSale.hpp"
#include "TimeAndSaleType.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Option Sale event represents a trade or another market event with the price
 * (for example, market open/close price, etc.) for each option symbol listed under the specified Underlying.
 * Option Sales are intended to provide information about option trades <b>in a continuous time slice</b> with
 * the additional metrics, like Option Volatility, Option Delta, and Underlying Price.
 *
 * <p>Option Sale events have unique @ref OptionSale::getIndex() "index" which can be used for later
 * correction/cancellation processing.
 *
 * Option sale data source provides a consistent view of the set of known option sales.
 * The corresponding information is carried in @ref OptionSale::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for option sale events, thus
 * @ref OptionSale::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * This event is implemented on top of QDS records `OptionSale`.
 */
class DXFCPP_EXPORT OptionSale final : public MarketEvent, public IndexedEvent {
    friend struct EventMapper;

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
        std::optional<std::string> exchangeSaleConditions{};
        std::int32_t flags{};
        double underlyingPrice = math::NaN;
        double volatility = math::NaN;
        double delta = math::NaN;
        std::optional<std::string> optionSymbol{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the OptionSale object
    using Ptr = std::shared_ptr<OptionSale>;

    /// The alias to a type of unique pointer to the OptionSale object
    using Unique = std::unique_ptr<OptionSale>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see OptionSale::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

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

    /// Creates new option sale event with default values.
    OptionSale() noexcept = default;

    /**
     * Creates new option sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OptionSale(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

    // MarketEvent methods

    /**
     * Changes event's symbol and returns the current option sale.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current option sale.
     */
    OptionSale &withEventSymbol(const std::string &eventSymbol) noexcept {
        MarketEvent::setEventSymbol(eventSymbol);

        return *this;
    }

    /**
     * Changes event's creation time and returns the current option sale.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current option sale.
     */
    OptionSale &withEventTime(std::int64_t eventTime) noexcept {
        MarketEvent::setEventTime(eventTime);

        return *this;
    }

    // IndexedEvent methods

    ///
    const IndexedEventSource &getSource() const & noexcept override {
        return IndexedEventSource::DEFAULT;
    }

    ///
    std::int32_t getEventFlags() const noexcept override {
        return data_.eventFlags;
    }

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override {
        return EventFlagsMask(data_.eventFlags);
    }

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override {
        data_.eventFlags = eventFlags;
    }

    /**
     * Changes transactional event flags and returns the current option sale.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current option sale.
     */
    OptionSale &withEventFlags(std::int32_t eventFlags) noexcept {
        OptionSale::setEventFlags(eventFlags);

        return *this;
    }

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override {
        data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    /**
     * Changes transactional event flags and returns the current option sale.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current option sale.
     */
    OptionSale &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        OptionSale::setEventFlags(eventFlags);

        return *this;
    }

    /**
     * Returns unique per-symbol index of this event.
     * The index is composed of @ref OptionSale::getTime() "time" and @ref OptionSale::getSequence() "sequence".
     * Changing either time or sequence changes event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override {
        return data_.index;
    }

    /**
     * Changes unique per-symbol index of this event.
     * The index is composed of @ref OptionSale::getTime() "time" and @ref OptionSale::getSequence() "sequence" and
     * invocation of this method changes time and sequence.
     * <b>Do not use this method directly.</b>
     * Change @ref OptionSale::setTime() "time" and/or @ref OptionSale::setSequence() "sequence".
     *
     * @param index the event index.
     * @see OptionSale::getIndex()
     */
    void setIndex(std::int64_t index) override {
        data_.index = index;
    }

    /**
     * Changes unique per-symbol index of this event.
     * Returns the current option sale.
     *
     * @param index unique per-symbol index of this option sale.
     * @return The current option sale.
     */
    OptionSale &withIndex(std::int64_t index) noexcept {
        OptionSale::setIndex(index);

        return *this;
    }

    // OptionSale methods

    /**
     * Returns time and sequence of this event packaged into single long value.
     *
     * @return time and sequence of this event.
     */
    std::int64_t getTimeSequence() const noexcept {
        return data_.timeSequence;
    }

    /**
     * Changes time and sequence of this event.
     * <b>Do not use this method directly.</b>
     * Change @ref OptionSale::setTime() "time" and/or @ref OptionSale::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept {
        data_.timeSequence = timeSequence;
    }

    /**
     * Changes time and sequence of this event.
     * Returns the current option sale.
     *
     * @param timeSequence the time and sequence.
     * @return The current option sale.
     */
    OptionSale &withTimeSequence(std::int64_t timeSequence) noexcept {
        OptionSale::setTimeSequence(timeSequence);

        return *this;
    }

    /**
     * Returns time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this event.
     */
    std::int64_t getTime() const noexcept {
        return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
               andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this event.
     */
    void setTime(std::int64_t time) noexcept {
        data_.timeSequence =
            orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                      sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                 getSequence());
    }

    /**
     * Changes timestamp of the event in milliseconds.
     * Returns the current option sale.
     *
     * @param time timestamp of the event in milliseconds.
     * @return The current option sale.
     */
    OptionSale &withTime(std::int64_t time) noexcept {
        OptionSale::setTime(time);

        return *this;
    }

    /**
     * Returns time of the original event in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the original event in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Changes time of the original event.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the original event in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) noexcept {
        setTime(time_nanos_util::getMillisFromNanos(timeNanos));
        data_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
    }

    /**
     * Changes time of this option sale and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this option sale in nanoseconds.
     * @return The current option sale.
     */
    OptionSale &withTimeNanos(std::int64_t timeNanos) noexcept {
        OptionSale::setTimeNanos(timeNanos);

        return *this;
    }

    /**
     * Changes microseconds and nanoseconds time part of the original event.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the original event.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        data_.timeNanoPart = timeNanoPart;
    }

    /**
     * Changes microseconds and nanoseconds time part of this option sale.
     * Returns the current option sale.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this option sale.
     * @return The current option sale.
     */
    OptionSale &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        OptionSale::setTimeNanoPart(timeNanoPart);

        return *this;
    }

    /**
     * Returns microseconds and nanoseconds time part of the original event.
     *
     * @return microseconds and nanoseconds time part of the original event.
     */
    std::int32_t getTimeNanoPart() const noexcept {
        return data_.timeNanoPart;
    }

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const noexcept {
        return static_cast<std::int32_t>(andOp(data_.timeSequence, MAX_SEQUENCE));
    }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @see ::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence) {
        assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
        }

        data_.timeSequence = orOp(andOp(data_.timeSequence, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     * Returns the current option sale.
     *
     * @param sequence the sequence.
     * @return The current option sale.
     * @see ::getSequence()
     */
    OptionSale &withSequence(std::int32_t sequence) noexcept {
        OptionSale::setSequence(sequence);

        return *this;
    }

    /**
     * Returns exchange code of this option sale event.
     *
     * @return exchange code of this option sale event.
     */
    std::int16_t getExchangeCode() const noexcept {
        return data_.exchangeCode;
    }

    /**
     * Returns exchange code of this option sale as UTF8 string.
     *
     * @return exchange code of this option sale as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept {
        // TODO: cache [EN-8231]

        return dxfcpp::utf16toUtf8String(data_.exchangeCode);
    }

    /**
     * Changes exchange code of this option sale event.
     *
     * @param exchangeCode exchange code of this option sale event.
     */
    void setExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of this option sale event.
     * Returns the current option sale.
     *
     * @param exchangeCode The exchange code of this option sale.
     * @return The current option sale.
     */
    OptionSale &withExchangeCode(char exchangeCode) noexcept {
        OptionSale::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Changes exchange code of this option sale event.
     *
     * @param exchangeCode exchange code of this option sale event.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept {
        data_.exchangeCode = exchangeCode;
    }

    /**
     * Changes exchange code of this option sale event.
     * Returns the current order.
     *
     * @param exchangeCode The exchange code of this option sale.
     * @return The current option sale.
     */
    OptionSale &withExchangeCode(std::int16_t exchangeCode) noexcept {
        OptionSale::setExchangeCode(exchangeCode);

        return *this;
    }

    /**
     * Returns price of this option sale event.
     *
     * @return price of this option sale event.
     */
    double getPrice() const noexcept {
        return data_.price;
    }

    /**
     * Changes price of this option sale event.
     *
     * @param price price of this option sale event.
     */
    void setPrice(double price) noexcept {
        data_.price = price;
    }

    /**
     * Changes price of this option sale event.
     * Returns the current option sale.
     *
     * @param price The price of this option sale.
     * @return The current option sale.
     */
    OptionSale &withPrice(double price) noexcept {
        OptionSale::setPrice(price);

        return *this;
    }

    /**
     * Returns size of this option sale event.
     *
     * @return size of this option sale event.
     */
    double getSize() const noexcept {
        return data_.size;
    }

    /**
     * Changes size of this option sale event.
     *
     * @param size size of this option sale event.
     */
    void setSize(double size) noexcept {
        data_.size = size;
    }

    /**
     * Returns the current bid price on the market when this option sale event had occurred.
     *
     * @return the current bid price on the market when this option sale event had occurred.
     */
    double getBidPrice() const noexcept {
        return data_.bidPrice;
    }

    /**
     * Changes the current bid price on the market when this option sale event had occurred.
     *
     * @param bidPrice the current bid price on the market when this option sale event had occurred.
     */
    void setBidPrice(double bidPrice) noexcept {
        data_.bidPrice = bidPrice;
    }

    /**
     * Returns the current ask price on the market when this option sale event had occurred.
     *
     * @return the current ask price on the market when this option sale event had occurred.
     */
    double getAskPrice() const noexcept {
        return data_.askPrice;
    }

    /**
     * Changes price of this time and sale event.the current ask price on the market when this option sale event had
     * occurred.
     *
     * @param askPrice the current ask price on the market when this option sale event had occurred.
     */
    void setAskPrice(double askPrice) noexcept {
        data_.askPrice = askPrice;
    }

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getExchangeSaleConditions() const & noexcept {
        if (!data_.exchangeSaleConditions) {
            return dxfcpp::String::NUL;
        }

        return data_.exchangeSaleConditions.value();
    }

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or `std::nullopt`.
     */
    const std::optional<std::string> &getExchangeSaleConditionsOpt() const & noexcept {
        return data_.exchangeSaleConditions;
    }

    /**
     * Changes sale conditions provided for this event by data feed.
     *
     * @param exchangeSaleConditions sale conditions.
     */
    void setExchangeSaleConditions(std::string exchangeSaleConditions) noexcept {
        data_.exchangeSaleConditions = std::move(exchangeSaleConditions);
    }

    /**
     * Returns TradeThroughExempt flag of this option sale event.
     *
     * @return TradeThroughExempt flag of this option sale event.
     */
    char getTradeThroughExempt() const noexcept {
        return static_cast<char>(
            static_cast<unsigned char>(getBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT)));
    }

    /**
     * Changes TradeThroughExempt flag of this option sale event.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this option sale event.
     */
    void setTradeThroughExempt(char tradeThroughExempt) {
        util::checkChar(tradeThroughExempt, TimeAndSale::TTE_MASK, "tradeThroughExempt");

        data_.flags = setBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT, tradeThroughExempt);
    }

    /**
     * Returns aggressor side of this option sale event.
     *
     * @return aggressor side of this option sale event.
     */
    const Side &getAggressorSide() const & noexcept {
        return Side::valueOf(getBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT));
    }

    /**
     * Changes aggressor side of this option sale event.
     *
     * @param side aggressor side of this option sale event.
     */
    void setAggressorSide(const Side &side) noexcept {
        data_.flags = setBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT, side.getCode());
    }

    /**
     * Returns whether this event represents a spread leg.
     *
     * @return `true` if this event represents a spread leg.
     */
    bool isSpreadLeg() const noexcept {
        return andOp(data_.flags, TimeAndSale::SPREAD_LEG) != 0;
    }

    /**
     * Changes whether this event represents a spread leg.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     */
    void setSpreadLeg(bool spreadLeg) noexcept {
        data_.flags =
            spreadLeg ? orOp(data_.flags, TimeAndSale::SPREAD_LEG) : andOp(data_.flags, ~TimeAndSale::SPREAD_LEG);
    }

    /**
     * Returns whether this event represents an extended trading hours sale.
     *
     * @return `true` if this event represents an extended trading hours sale.
     */
    bool isExtendedTradingHours() const noexcept {
        return andOp(data_.flags, TimeAndSale::ETH) != 0;
    }

    /**
     * Changes whether this event represents an extended trading hours sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     */
    void setExtendedTradingHours(bool extendedTradingHours) noexcept {
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
    bool isValidTick() const noexcept {
        return andOp(data_.flags, TimeAndSale::VALID_TICK) != 0;
    }

    /**
     * Changes whether this event represents a valid intraday tick.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     */
    void setValidTick(bool validTick) noexcept {
        data_.flags =
            validTick ? orOp(data_.flags, TimeAndSale::VALID_TICK) : andOp(data_.flags, ~TimeAndSale::VALID_TICK);
    }

    /**
     * Returns type of this option sale event.
     *
     * @return type of this option sale event.
     */
    const TimeAndSaleType &getType() const & noexcept {
        return TimeAndSaleType::valueOf(getBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT));
    }

    /**
     * Changes type of this option sale event.
     *
     * @param type type of this option sale event.
     */
    void setType(const TimeAndSaleType &type) noexcept {
        data_.flags = setBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT, type.getCode());
    }

    /**
     * Returns whether this is a new event (not cancellation or correction).
     * It is `true` for newly created option sale event.
     *
     * @return `true` if this is a new event (not cancellation or correction).
     */
    bool isNew() const noexcept {
        return getType() == TimeAndSaleType::NEW;
    }

    /**
     * Returns whether this is a correction of a previous event.
     * It is `false` for newly created option sale event.
     *
     * @return `true` if this is a correction of a previous event
     */
    bool isCorrection() const noexcept {
        return getType() == TimeAndSaleType::CORRECTION;
    }

    /**
     * Returns whether this is a cancellation of a previous event.
     * It is `false` for newly created option sale event.
     * @return `true` if this is a cancellation of a previous event
     */
    bool isCancel() const noexcept {
        return getType() == TimeAndSaleType::CANCEL;
    }

    /**
     * Returns underlying price at the time of this option sale event.
     *
     * @return underlying price at the time of this option sale event.
     */
    double getUnderlyingPrice() const noexcept {
        return data_.underlyingPrice;
    }

    /**
     * Changes underlying price at the time of this option sale event.
     *
     * @param underlyingPrice underlying price at the time of this option sale event.
     */
    void setUnderlyingPrice(double underlyingPrice) noexcept {
        data_.underlyingPrice = underlyingPrice;
    }

    /**
     * Returns Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @return Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    double getVolatility() const noexcept {
        return data_.volatility;
    }

    /**
     * Changes Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @param volatility Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    void setVolatility(double volatility) noexcept {
        data_.volatility = volatility;
    }

    /**
     * Return option delta at the time of this option sale event.
     * Delta is the first derivative of an option price by an underlying price.
     *
     * @return option delta at the time of this option sale event.
     */
    double getDelta() const noexcept {
        return data_.delta;
    }

    /**
     * Changes option delta at the time of this option sale event.
     *
     * @param delta option delta at the time of this option sale event.
     */
    void setDelta(double delta) noexcept {
        data_.delta = delta;
    }

    /**
     * Returns option symbol of this event.
     *
     * @return option symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getOptionSymbol() const & noexcept {
        if (!data_.optionSymbol) {
            return dxfcpp::String::NUL;
        }

        return data_.optionSymbol.value();
    }

    /**
     * Returns option symbol of this event.
     *
     * @return option symbol of this event or std::nullopt
     */
    const std::optional<std::string> &getOptionSymbolOpt() const & noexcept {
        return data_.optionSymbol;
    }

    /**
     * Changes option symbol of this event.
     *
     * @param optionSymbol option symbol of this event.
     */
    void setOptionSymbol(std::string optionSymbol) noexcept {
        data_.optionSymbol = std::move(optionSymbol);
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