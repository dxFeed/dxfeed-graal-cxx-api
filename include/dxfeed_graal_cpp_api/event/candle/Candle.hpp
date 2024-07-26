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

#include "../../exceptions/InvalidArgumentException.hpp"

#include "../EventType.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "CandleSymbol.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Candle event with open, high, low, close prices and other information for a specific period.
 * Candles are build with a specified CandlePeriod using a specified CandlePrice type with a data taken from the
 * specified CandleExchange from the specified CandleSession with further details of aggregation provided by
 * CandleAlignment.
 *
 * <p> Event symbol of the candle is represented with CandleSymbol class.
 * Since the `Candle is a time-series event, it is typically subscribed to using DXFeedTimeSeriesSubscription class
 * that handles the necessarily wrapping of the symbol into TimeSeriesSubscriptionSymbol to specify a subscription
 * time range.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some candle sources provide a consistent view of the set of known candles.
 * The corresponding information is carried in @ref Candle::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for candles, thus @ref Candle::getSource() "source"
 * property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p>TimeSeriesEventModel class handles all the snapshot and transaction logic and conveniently represents a list
 * current of time-series events order by their @ref Candle::getTime() "time".
 * It relies on the code of AbstractIndexedEventModel to handle this logic.
 * Use the source code of AbstractIndexedEventModel for clarification on transactions and snapshot logic.
 *
 * <h3>Publishing Candles</h3>
 *
 * Publishing of candle events follows the general rules explained in TimeSeriesEvent class documentation.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `TradeHistory` for tick candles with CandlePeriod::TICK,
 * records `Trade.<period>` for a certain set of popular periods, and QDS record `Candle` for arbitrary custom
 * periods, with a set of `Candle{<attributes>}` records for a popular combinations of custom candle symbol attributes
 * like CandlePrice for an efficient support of bid-ask charting.
 */
class DXFCPP_EXPORT Candle final : public EventTypeWithSymbol<CandleSymbol>,
                                   public TimeSeriesEvent,
                                   public LastingEvent {
    friend struct EventMapper;

    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    /*
     * EventFlags property has several significant bits that are packed into an integer in the following way:
     *    31..7    6    5    4    3    2    1    0
     * +---------+----+----+----+----+----+----+----+
     * |         | SM |    | SS | SE | SB | RE | TX |
     * +---------+----+----+----+----+----+----+----+
     */

    std::optional<CandleSymbol> eventSymbol_{};

    struct Data {
        // dxfg_event_type_t event_type;
        // const char *event_symbol;
        std::int64_t eventTime{};
        std::int32_t eventFlags{};
        std::int64_t index{};
        std::int64_t count{};
        double open = math::NaN;
        double high = math::NaN;
        double low = math::NaN;
        double close = math::NaN;
        double volume = math::NaN;
        double vwap = math::NaN;
        double bidVolume = math::NaN;
        double askVolume = math::NaN;
        double impVolatility = math::NaN;
        double openInterest = math::NaN;
    };

    Data data_{};

    void fillData(void *graalNative);
    void fillGraalData(void *graalNative) const;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the Candle object
    using Ptr = std::shared_ptr<Candle>;

    /// The alias to a type of unique pointer to the Candle object
    using Unique = std::unique_ptr<Candle>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see Candle::setSequence()
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

    ///
    void assign(std::shared_ptr<EventType> event) override;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates new candle with default values.
     */
    Candle() noexcept = default;

    /**
     * Creates new candle with the specified candle event symbol.
     * @param eventSymbol candle event symbol.
     */
    explicit Candle(CandleSymbol eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {
    }

    // EventType methods

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or dxfcpp::CandleSymbol::NUL (`dxfcpp::CandleSymbol{"<null>"}`)
     */
    const CandleSymbol &getEventSymbol() const & noexcept override {
        if (!eventSymbol_) {
            return CandleSymbol::NUL;
        }

        return eventSymbol_.value();
    }

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or `std::nullopt`.
     */
    const std::optional<CandleSymbol> &getEventSymbolOpt() const & noexcept override {
        return eventSymbol_;
    }

    ///
    void setEventSymbol(const CandleSymbol &eventSymbol) noexcept override {
        eventSymbol_ = eventSymbol;
    }

    /**
     * Changes event's symbol and returns the current candle.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current candle.
     */
    Candle &withEventSymbol(const CandleSymbol &eventSymbol) noexcept {
        Candle::setEventSymbol(eventSymbol);

        return *this;
    }

    ///
    std::int64_t getEventTime() const noexcept override {
        return data_.eventTime;
    }

    ///
    void setEventTime(std::int64_t eventTime) noexcept override {
        data_.eventTime = eventTime;
    }

    /**
     * Changes event's creation time and returns the current candle.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current candle.
     */
    Candle &withEventTime(std::int64_t eventTime) noexcept {
        Candle::setEventTime(eventTime);

        return *this;
    }

    // IndexedEvent methods

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
     * Changes transactional event flags and returns the current candle.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current candle.
     */
    Candle &withEventFlags(std::int32_t eventFlags) noexcept {
        Candle::setEventFlags(eventFlags);

        return *this;
    }

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override {
        data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    /**
     * Changes transactional event flags and returns the current candle.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current candle.
     */
    Candle &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        Candle::setEventFlags(eventFlags);

        return *this;
    }

    ///
    void setIndex(std::int64_t index) override {
        data_.index = index;
    }

    /**
     * Changes unique per-symbol index of this event.
     * Returns the current candle.
     *
     * @param index unique per-symbol index of this candle.
     * @return The current candle.
     */
    Candle &withIndex(std::int64_t index) noexcept {
        Candle::setIndex(index);

        return *this;
    }

    ///
    std::int64_t getIndex() const noexcept override {
        return data_.index;
    }

    /**
     * Returns timestamp of the event in milliseconds.
     *
     * @return timestamp of the event in milliseconds
     */
    std::int64_t getTime() const noexcept override {
        return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes timestamp of the event in milliseconds.
     *
     * @param time timestamp of the event in milliseconds.
     * @see Candle::getTime()
     */
    void setTime(std::int64_t time) noexcept {
        data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                                sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                           getSequence());
    }

    /**
     * Changes timestamp of the event in milliseconds.
     * Returns the current candle.
     *
     * @param time timestamp of the event in milliseconds.
     * @return The current candle.
     */
    Candle &withTime(std::int64_t time) noexcept {
        Candle::setTime(time);

        return *this;
    }

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref Candle::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to Candle::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const noexcept {
        return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
    }

    /**
     * Changes @ref Candle::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @see Candle::getSequence()
     * @throws InvalidArgumentException if sequence is below zero or above ::MAX_SEQUENCE.
     */
    void setSequence(std::int32_t sequence) {
        assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
        }

        data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Changes @ref Candle::getSequence() "sequence number" of this event.
     * Returns the current candle.
     *
     * @param sequence the sequence.
     * @return The current candle.
     * @see Candle::getSequence()
     */
    Candle &withSequence(std::int32_t sequence) noexcept {
        Candle::setSequence(sequence);

        return *this;
    }

    /**
     * Returns total number of original trade (or quote) events in this candle.
     * @return Total number of original trade (or quote) events in this candle.
     */
    std::int64_t getCount() const noexcept {
        return data_.count;
    }

    /**
     * Changes total number of original trade (or quote) events in this candle.
     * @param count Total number of original trade (or quote) events in this candle.
     */
    void setCount(std::int64_t count) noexcept {
        data_.count = count;
    }

    /**
     * Changes total number of original trade (or quote) events in this candle.
     * Returns the current candle.
     *
     * @param count Total number of original trade (or quote) events in this candle.
     * @return The current candle.
     */
    Candle &withCount(std::int64_t count) noexcept {
        Candle::setCount(count);

        return *this;
    }

    /**
     * Returns the first (open) price of this candle.
     * @return The first (open) price of this candle.
     */
    double getOpen() const noexcept {
        return data_.open;
    }

    /**
     * Changes the first (open) price of this candle.
     * @param open The first (open) price of this candle.
     */
    void setOpen(double open) noexcept {
        data_.open = open;
    }

    /**
     * Changes the first (open) price of this candle.
     * Returns the current candle.
     *
     * @param open The first (open) price of this candle.
     * @return The current candle.
     */
    Candle &withOpen(double open) noexcept {
        Candle::setOpen(open);

        return *this;
    }

    /**
     * Returns the maximal (high) price of this candle.
     * @return The maximal (high) price of this candle.
     */
    double getHigh() const noexcept {
        return data_.high;
    }

    /**
     * Changes the maximal (high) price of this candle.
     * @param high The maximal (high) price of this candle.
     */
    void setHigh(double high) noexcept {
        data_.high = high;
    }

    /**
     * Changes the maximal (high) price of this candle.
     * Returns the current candle.
     *
     * @param high The maximal (high) price of this candle.
     * @return The current candle.
     */
    Candle &withHigh(double high) noexcept {
        Candle::setHigh(high);

        return *this;
    }

    /**
     * Returns the minimal (low) price of this candle.
     * @return The minimal (low) price of this candle.
     */
    double getLow() const noexcept {
        return data_.low;
    }

    /**
     * Changes the minimal (low) price of this candle.
     * @param low The minimal (low) price of this candle.
     */
    void setLow(double low) noexcept {
        data_.low = low;
    }

    /**
     * Changes the minimal (low) price of this candle.
     * Returns the current candle.
     *
     * @param low The minimal (low) price of this candle.
     * @return The current candle.
     */
    Candle &withLow(double low) noexcept {
        Candle::setLow(low);

        return *this;
    }

    /**
     * Returns the last (close) price of this candle.
     * @return The last (close) price of this candle.
     */
    double getClose() const noexcept {
        return data_.close;
    }

    /**
     * Changes the last (close) price of this candle.
     * @param close The last (close) price of this candle.
     */
    void setClose(double close) noexcept {
        data_.close = close;
    }

    /**
     * Changes the last (close) price of this candle.
     * Returns the current candle.
     *
     * @param close The last (close) price of this candle.
     * @return The current candle.
     */
    Candle &withClose(double close) noexcept {
        Candle::setClose(close);

        return *this;
    }

    /**
     * Returns total volume in this candle.
     * @return Total volume in this candle.
     */
    double getVolume() const noexcept {
        return data_.volume;
    }

    /**
     * Changes total volume in this candle.
     * @param volume Total volume in this candle.
     */
    void setVolume(double volume) noexcept {
        data_.volume = volume;
    }

    /**
     * Changes total volume in this candle.
     * Returns the current candle.
     *
     * @param volume Total volume in this candle.
     * @return The current candle.
     */
    Candle &withVolume(double volume) noexcept {
        Candle::setVolume(volume);

        return *this;
    }

    /**
     * Returns volume-weighted average price (VWAP) in this candle.
     * Total turnover in this candle can be computed with <code>getVWAP() * @ref Candle::getVolume()
     * "getVolume"()</code>.
     * @return Volume-weighted average price (VWAP) in this candle.
     */
    double getVWAP() const noexcept {
        return data_.vwap;
    }

    /**
     * Changes volume-weighted average price (VWAP) in this candle.
     * @param vwap Volume-weighted average price (VWAP) in this candle.
     */
    void setVWAP(double vwap) noexcept {
        data_.vwap = vwap;
    }

    /**
     * Changes volume-weighted average price (VWAP) in this candle.
     * Returns the current candle.
     *
     * @param vwap Volume-weighted average price (VWAP) in this candle.
     * @return The current candle.
     */
    Candle &withVWAP(double vwap) noexcept {
        Candle::setVWAP(vwap);

        return *this;
    }

    /**
     * Returns bid volume in this candle.
     * @return Bid volume in this candle.
     */
    double getBidVolume() const noexcept {
        return data_.bidVolume;
    }

    /**
     * Changes bid volume in this candle.
     * @param bidVolume Bid volume in this candle.
     */
    void setBidVolume(double bidVolume) noexcept {
        data_.bidVolume = bidVolume;
    }

    /**
     * Changes bid volume in this candle.
     * Returns the current candle.
     *
     * @param bidVolume Bid volume in this candle.
     * @return The current candle.
     */
    Candle &withBidVolume(double bidVolume) noexcept {
        Candle::setBidVolume(bidVolume);

        return *this;
    }

    /**
     * Returns ask volume in this candle.
     * @return Ask volume in this candle.
     */
    double getAskVolume() const noexcept {
        return data_.askVolume;
    }

    /**
     * Changes ask volume in this candle.
     * @param askVolume Ask volume in this candle.
     */
    void setAskVolume(double askVolume) noexcept {
        data_.askVolume = askVolume;
    }

    /**
     * Changes ask volume in this candle.
     * Returns the current candle.
     *
     * @param askVolume Ask volume in this candle.
     * @return The current candle.
     */
    Candle &withAskVolume(double askVolume) noexcept {
        Candle::setAskVolume(askVolume);

        return *this;
    }

    /**
     * Returns the implied volatility.
     * @return The implied volatility.
     */
    double getImpVolatility() const noexcept {
        return data_.impVolatility;
    }

    /**
     * Changes the implied volatility.
     * @param impVolatility The implied volatility.
     */
    void setImpVolatility(double impVolatility) {
        data_.impVolatility = impVolatility;
    }

    /**
     * Changes implied volatility.
     * Returns the current candle.
     *
     * @param impVolatility The implied volatility.
     * @return The current candle.
     */
    Candle &withImpVolatility(double impVolatility) noexcept {
        Candle::setImpVolatility(impVolatility);

        return *this;
    }

    /**
     * Returns the open interest.
     * @return The open interest.
     */
    double getOpenInterest() const noexcept {
        return data_.openInterest;
    }

    /**
     * Changes the open interest.
     * @param openInterest The open interest.
     */
    void setOpenInterest(double openInterest) noexcept {
        data_.openInterest = openInterest;
    }

    /**
     * Changes the open interest.
     * Returns the current candle.
     *
     * @param openInterest The open interest.
     * @return The current candle.
     */
    Candle &withOpenInterest(double openInterest) noexcept {
        Candle::setOpenInterest(openInterest);

        return *this;
    }

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()