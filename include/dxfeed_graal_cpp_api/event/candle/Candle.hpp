// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "../../internal/Common.hpp"
#include "../EventType.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "CandleSymbol.hpp"

namespace dxfcpp {

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
 * Multiple event sources for the same symbol are not supported for candles, thus @ref Candle::getSource() "source" property
 * is always @ref IndexedEventSource::DEFAULT "DEFAULT".
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

    CandleSymbol eventSymbol_{};

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

    void fillData(void *graalNative) noexcept;
    void fillGraalData(void *graalNative) const noexcept;
    static void freeGraalData(void *graalNative) noexcept;

    static std::shared_ptr<Candle> fromGraal(void *graalNative) noexcept;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void* toGraal() const noexcept;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     */
    static void freeGraal(void* graalNative) noexcept;

  public:
    /**
     * Maximum allowed sequence value.
     *
     * @see Candle::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    static const EventTypeEnum &TYPE;

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

    ///
    const CandleSymbol &getEventSymbol() const noexcept override {
        return eventSymbol_;
    }

    ///
    void setEventSymbol(const CandleSymbol &eventSymbol) noexcept override {
        eventSymbol_ = eventSymbol;
    }

    std::int64_t getEventTime() const noexcept override {
        return data_.eventTime;
    }

    void setEventTime(std::int64_t eventTime) noexcept override {
        data_.eventTime = eventTime;
    }

    std::string toString() const noexcept override;

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

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override {
        data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    ///
    void setIndex(std::int64_t index) noexcept override {
        data_.index = index;
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
        data_.index = orOp(orOp(sal(time_util::getSecondsFromTime(time), SECONDS_SHIFT),
                                sal(time_util::getMillisFromTime(time), MILLISECONDS_SHIFT)),
                           getSequence());
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
     */
    void setSequence(int sequence) noexcept {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns total number of original trade (or quote) events in this candle.
     * @return total number of original trade (or quote) events in this candle.
     */
    std::int64_t getCount() const noexcept {
        return data_.count;
    }

    /**
     * Changes total number of original trade (or quote) events in this candle.
     * @param count total number of original trade (or quote) events in this candle.
     */
    void setCount(std::int64_t count) noexcept{
        data_.count = count;
    }

    /**
     * Returns the first (open) price of this candle.
     * @return the first (open) price of this candle.
     */
    double getOpen() const noexcept {
        return data_.open;
    }

    /**
     * Changes the first (open) price of this candle.
     * @param open the first (open) price of this candle.
     */
    void setOpen(double open) noexcept{
        data_.open = open;
    }

    /**
     * Returns the maximal (high) price of this candle.
     * @return the maximal (high) price of this candle.
     */
    double getHigh() const noexcept {
        return data_.high;
    }

    /**
     * Changes the maximal (high) price of this candle.
     * @param high the maximal (high) price of this candle.
     */
    void setHigh(double high) noexcept{
        data_.high = high;
    }

    /**
     * Returns the minimal (low) price of this candle.
     * @return the minimal (low) price of this candle.
     */
    double getLow() const noexcept {
        return data_.low;
    }

    /**
     * Changes the minimal (low) price of this candle.
     * @param low the minimal (low) price of this candle.
     */
    void setLow(double low) noexcept{
        data_.low = low;
    }

    /**
     * Returns the last (close) price of this candle.
     * @return the last (close) price of this candle.
     */
    double getClose() const noexcept {
        return data_.close;
    }

    /**
     * Changes the last (close) price of this candle.
     * @param close the last (close) price of this candle.
     */
    void setClose(double close) noexcept{
        data_.close = close;
    }

    /**
     * Returns total volume in this candle.
     * @return total volume in this candle.
     */
    double getVolume() const noexcept {
        return data_.volume;
    }

    /**
     * Changes total volume in this candle.
     * @param volume total volume in this candle.
     */
    void setVolume(double volume) noexcept{
        data_.volume = volume;
    }

    /**
     * Returns volume-weighted average price (VWAP) in this candle.
     * Total turnover in this candle can be computed with <code>getVWAP() * @ref Candle::getVolume() "getVolume"()</code>.
     * @return volume-weighted average price (VWAP) in this candle.
     */
        double getVWAP() const noexcept {
        return data_.vwap;
    }

    /**
     * Changes volume-weighted average price (VWAP) in this candle.
     * @param vwap volume-weighted average price (VWAP) in this candle.
     */
    void setVWAP(double vwap) noexcept{
        data_.vwap = vwap;
    }

    /**
     * Returns bid volume in this candle.
     * @return bid volume in this candle.
     */
    double getBidVolume() const noexcept {
        return data_.bidVolume;
    }

    /**
     * Changes bid volume in this candle.
     * @param bidVolume bid volume in this candle.
     */
    void setBidVolume(double bidVolume) noexcept{
        data_.bidVolume = bidVolume;
    }

    /**
     * Returns ask volume in this candle.
     * @return ask volume in this candle.
     */
    double getAskVolume() const noexcept {
        return data_.askVolume;
    }

    /**
     * Changes ask volume in this candle.
     * @param askVolume ask volume in this candle.
     */
    void setAskVolume(double askVolume) noexcept{
        data_.askVolume = askVolume;
    }

    /**
     * Returns implied volatility.
     * @return implied volatility.
     */
    double getImpVolatility() const noexcept {
        return data_.impVolatility;
    }

    /**
     * Changes implied volatility.
     * @param impVolatility implied volatility.
     */
    void setImpVolatility(double impVolatility) {
        data_.impVolatility = impVolatility;
    }

    /**
     * Returns open interest.
     * @return open interest.
     */
    double getOpenInterest() const noexcept {
        return data_.openInterest;
    }

    /**
     * Changes open interest.
     * @param openInterest open interest.
     */
    void setOpenInterest(double openInterest) noexcept{
        data_.openInterest = openInterest;
    }
};

} // namespace dxfcpp