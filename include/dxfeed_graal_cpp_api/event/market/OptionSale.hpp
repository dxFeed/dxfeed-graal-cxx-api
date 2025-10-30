// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

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
#include "TimeAndSaleType.hpp"
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>

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
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
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
     * Creates a new option sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OptionSale(const StringLike &eventSymbol) noexcept;

    // MarketEvent methods

    /**
     * Changes an event's symbol and returns the current option sale.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current option sale.
     */
    OptionSale &withEventSymbol(const StringLike &eventSymbol) noexcept;

    /**
     * Changes event's creation time and returns the current option sale.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current option sale.
     */
    OptionSale &withEventTime(std::int64_t eventTime) noexcept;

    // IndexedEvent methods

    ///
    const IndexedEventSource &getSource() const & noexcept override;

    ///
    std::int32_t getEventFlags() const noexcept override;

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override;

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current option sale.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current option sale.
     */
    OptionSale &withEventFlags(std::int32_t eventFlags) noexcept;

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current option sale.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current option sale.
     */
    OptionSale &withEventFlags(const EventFlagsMask &eventFlags) noexcept;

    /**
     * Returns a unique per-symbol index of this event.
     * The index is composed of @ref OptionSale::getTime() "time" and @ref OptionSale::getSequence() "sequence".
     * Changing either time or sequence changes the event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override;

    /**
     * Changes the unique per-symbol index of this event.
     * The index is composed of @ref OptionSale::getTime() "time" and @ref OptionSale::getSequence() "sequence" and
     * invocation of this method changes time and sequence.
     * <b>Do not use this method directly.</b>
     * Change @ref OptionSale::setTime() "time" and/or @ref OptionSale::setSequence() "sequence".
     *
     * @param index the event index.
     * @see OptionSale::getIndex()
     */
    void setIndex(std::int64_t index) override;

    /**
     * Changes unique per-symbol index of this event.
     * Returns the current option sale.
     *
     * @param index unique per-symbol index of this option sale.
     * @return The current option sale.
     */
    OptionSale &withIndex(std::int64_t index) noexcept;

    // OptionSale methods

    /**
     * Returns time and sequence of this event packaged into a single long value.
     *
     * @return time and sequence of this event.
     */
    std::int64_t getTimeSequence() const noexcept;

    /**
     * Changes time and sequence of this event.
     * <b>Do not use this method directly.</b>
     * Change @ref OptionSale::setTime() "time" and/or @ref OptionSale::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Changes time and sequence of this event.
     * Returns the current option sale.
     *
     * @param timeSequence the time and sequence.
     * @return The current option sale.
     */
    OptionSale &withTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Returns time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this event.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Changes time of this event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this event.
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Changes the timestamp of the event in milliseconds.
     * Returns the current option sale.
     *
     * @param time timestamp of the event in milliseconds.
     * @return The current option sale.
     */
    OptionSale &withTime(std::int64_t time) noexcept;

    /**
     * Returns time of the original event in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the original event in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept;

    /**
     * Changes time of the original event.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the original event in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) noexcept;

    /**
     * Changes the time of this option sale and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this option sale in nanoseconds.
     * @return The current option sale.
     */
    OptionSale &withTimeNanos(std::int64_t timeNanos) noexcept;

    /**
     * Changes microseconds and nanoseconds time part of the original event.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the original event.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept;

    /**
     * Changes microseconds and nanoseconds time part of this option sale.
     * Returns the current option sale.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this option sale.
     * @return The current option sale.
     */
    OptionSale &withTimeNanoPart(std::int32_t timeNanoPart) noexcept;

    /**
     * Returns microseconds and nanoseconds time part of the original event.
     *
     * @return microseconds and nanoseconds time part of the original event.
     */
    std::int32_t getTimeNanoPart() const noexcept;

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @see ::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence);

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     * Returns the current option sale.
     *
     * @param sequence the sequence.
     * @return The current option sale.
     * @see ::getSequence()
     */
    OptionSale &withSequence(std::int32_t sequence) noexcept;

    /**
     * Returns exchange code of this option sale event.
     *
     * @return exchange code of this option sale event.
     */
    std::int16_t getExchangeCode() const noexcept;

    /**
     * Returns exchange code of this option sale as UTF8 string.
     *
     * @return exchange code of this option sale as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept;

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
    OptionSale &withExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of this option sale event.
     *
     * @param exchangeCode exchange code of this option sale event.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Changes exchange code of this option sale event.
     * Returns the current order.
     *
     * @param exchangeCode The exchange code of this option sale.
     * @return The current option sale.
     */
    OptionSale &withExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Returns price of this option sale event.
     *
     * @return price of this option sale event.
     */
    double getPrice() const noexcept;

    /**
     * Changes price of this option sale event.
     *
     * @param price price of this option sale event.
     */
    void setPrice(double price) noexcept;

    /**
     * Changes price of this option sale event.
     * Returns the current option sale.
     *
     * @param price The price of this option sale.
     * @return The current option sale.
     */
    OptionSale &withPrice(double price) noexcept;

    /**
     * Returns the size of this option sale event.
     *
     * @return size of this option sale event.
     */
    double getSize() const noexcept;

    /**
     * Changes the size of this option sale event.
     *
     * @param size size of this option sale event.
     */
    void setSize(double size) noexcept;

    /**
     * Changes the size of this option sale event.
     * Returns the current option sale.
     *
     * @param size size of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withSize(double size) noexcept;

    /**
     * Returns the current bid price on the market when this option sale event had occurred.
     *
     * @return the current bid price on the market when this option sale event had occurred.
     */
    double getBidPrice() const noexcept;

    /**
     * Changes the current bid price on the market when this option sale event had occurred.
     *
     * @param bidPrice the current bid price on the market when this option sale event had occurred.
     */
    void setBidPrice(double bidPrice) noexcept;

    /**
     * Changes the current bid price on the market when this option sale event had occurred.
     * Returns the current option sale.
     *
     * @param bidPrice the current bid price on the market when this option sale event had occurred.
     * @return The current option sale.
     */
    OptionSale &withBidPrice(double bidPrice) noexcept;

    /**
     * Returns the current ask price on the market when this option sale event had occurred.
     *
     * @return the current ask price on the market when this option sale event had occurred.
     */
    double getAskPrice() const noexcept;

    /**
     * Changes the current ask price on the market when this option sale event had occurred.
     *
     * @param askPrice the current ask price on the market when this option sale event had occurred.
     */
    void setAskPrice(double askPrice) noexcept;

    /**
     * Changes the current ask price on the market when this option sale event had occurred.
     * Returns the current option sale.
     *
     * @param askPrice the current ask price on the market when this option sale event had occurred.
     * @return The current option sale.
     */
    OptionSale &withAskPrice(double askPrice) noexcept;

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getExchangeSaleConditions() const & noexcept;

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or `std::nullopt`.
     */
    const std::optional<std::string> &getExchangeSaleConditionsOpt() const & noexcept;

    /**
     * Changes sale conditions provided for this event by data feed.
     *
     * @param exchangeSaleConditions sale conditions.
     */
    void setExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept;

    /**
     * Changes sale conditions provided for this event by data feed.
     * Returns the current option sale.
     *
     * @param exchangeSaleConditions sale conditions.
     * @return The current option sale.
     */
    OptionSale &withExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept;

    /**
     * Returns TradeThroughExempt flag of this option sale event.
     *
     * @return TradeThroughExempt flag of this option sale event.
     */
    char getTradeThroughExempt() const noexcept;

    /**
     * Changes TradeThroughExempt flag of this option sale event.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this option sale event.
     */
    void setTradeThroughExempt(char tradeThroughExempt);

    /**
     * Changes TradeThroughExempt flag of this option sale event.
     * Returns the current option sale.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withTradeThroughExempt(char tradeThroughExempt);

    /**
     * Returns the aggressor side of this option sale event.
     *
     * @return aggressor side of this option sale event.
     */
    const Side &getAggressorSide() const & noexcept;

    /**
     * Changes aggressor side of this option sale event.
     *
     * @param side aggressor side of this option sale event.
     */
    void setAggressorSide(const Side &side) noexcept;

    /**
     * Changes aggressor side of this option sale event.
     * Returns the current option sale.
     *
     * @param side aggressor side of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withAggressorSide(const Side &side) noexcept;

    /**
     * Returns whether this event represents a spread leg.
     *
     * @return `true` if this event represents a spread leg.
     */
    bool isSpreadLeg() const noexcept;

    /**
     * Changes whether this event represents a spread leg.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     */
    void setSpreadLeg(bool spreadLeg) noexcept;

    /**
     * Changes whether this event represents a spread leg.
     * Returns the current option sale.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     * @return The current option sale.
     */
    OptionSale &withSpreadLeg(bool spreadLeg) noexcept;

    /**
     * Returns whether this event represents an extended trading hours sale.
     *
     * @return `true` if this event represents an extended trading hours sale.
     */
    bool isExtendedTradingHours() const noexcept;

    /**
     * Changes whether this event represents an extended trading hours sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     */
    void setExtendedTradingHours(bool extendedTradingHours) noexcept;

    /**
     * Changes whether this event represents an extended trading hours sale.
     * Returns the current option sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     * @return The current option sale.
     */
    OptionSale &withExtendedTradingHours(bool extendedTradingHours) noexcept;

    /**
     * Returns whether this event represents a valid intraday tick.
     * Note that a correction for a previously distributed valid tick represents a new valid tick itself,
     * but a cancellation of a previous valid tick does not.
     *
     * @return `true` if this event represents a valid intraday tick.
     */
    bool isValidTick() const noexcept;

    /**
     * Changes whether this event represents a valid intraday tick.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     */
    void setValidTick(bool validTick) noexcept;

    /**
     * Changes whether this event represents a valid intraday tick.
     * Returns the current option sale.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     * @return The current option sale.
     */
    OptionSale &withValidTick(bool validTick) noexcept;

    /**
     * Returns the type of this option sale event.
     *
     * @return type of this option sale event.
     */
    const TimeAndSaleType &getType() const & noexcept;

    /**
     * Changes the type of this option sale event.
     *
     * @param type type of this option sale event.
     */
    void setType(const TimeAndSaleType &type) noexcept;

    /**
     * Changes the type of this option sale event.
     * Returns the current option sale.
     *
     * @param type type of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withType(const TimeAndSaleType &type) noexcept;

    /**
     * Returns whether this is a new event (not cancellation or correction).
     * It is `true` for a newly created option sale event.
     *
     * @return `true` if this is a new event (not cancellation or correction).
     */
    bool isNew() const noexcept;

    /**
     * Returns whether this is a correction of a previous event.
     * It is `false` for a newly created option sale event.
     *
     * @return `true` if this is a correction of a previous event
     */
    bool isCorrection() const noexcept;

    /**
     * Returns whether this is a cancellation of a previous event.
     * It is `false` for a newly created option sale event.
     * @return `true` if this is a cancellation of a previous event
     */
    bool isCancel() const noexcept;

    /**
     * Returns the underlying price at the time of this option sale event.
     *
     * @return underlying price at the time of this option sale event.
     */
    double getUnderlyingPrice() const noexcept;

    /**
     * Changes underlying price at the time of this option sale event.
     *
     * @param underlyingPrice underlying price at the time of this option sale event.
     */
    void setUnderlyingPrice(double underlyingPrice) noexcept;

    /**
     * Changes underlying price at the time of this option sale event.
     * Returns the current option sale.
     *
     * @param underlyingPrice underlying price at the time of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withUnderlyingPrice(double underlyingPrice) noexcept;

    /**
     * Returns Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @return Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    double getVolatility() const noexcept;

    /**
     * Changes Black-Scholes implied volatility of the option at the time of this option sale event.
     *
     * @param volatility Black-Scholes implied volatility of the option at the time of this option sale event.
     */
    void setVolatility(double volatility) noexcept;

    /**
     * Changes Black-Scholes implied volatility of the option at the time of this option sale event.
     * Returns the current option sale.
     *
     * @param volatility Black-Scholes implied volatility of the option at the time of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withVolatility(double volatility) noexcept;

    /**
     * Return option delta at the time of this option sale event.
     * Delta is the first derivative of an option price by an underlying price.
     *
     * @return option delta at the time of this option sale event.
     */
    double getDelta() const noexcept;

    /**
     * Changes option delta at the time of this option sale event.
     *
     * @param delta option delta at the time of this option sale event.
     */
    void setDelta(double delta) noexcept;

    /**
     * Changes option delta at the time of this option sale event.
     * Returns the current option sale.
     *
     * @param delta option delta at the time of this option sale event.
     * @return The current option sale.
     */
    OptionSale &withDelta(double delta) noexcept;

    /**
     * Returns option symbol of this event.
     *
     * @return option symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getOptionSymbol() const & noexcept;

    /**
     * Returns option symbol of this event.
     *
     * @return option symbol of this event or `std::nullopt`
     */
    const std::optional<std::string> &getOptionSymbolOpt() const & noexcept;

    /**
     * Changes option symbol of this event.
     *
     * @param optionSymbol option symbol of this event.
     */
    void setOptionSymbol(const StringLike &optionSymbol) noexcept;

    /**
     * Changes option symbol of this event.
     * Returns the current option sale.
     *
     * @param optionSymbol option symbol of this event.
     * @return The current option sale.
     */
    OptionSale &withOptionSymbol(const StringLike &optionSymbol) noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()