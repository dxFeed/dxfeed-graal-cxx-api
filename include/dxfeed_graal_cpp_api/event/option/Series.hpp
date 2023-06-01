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
#include "../market/MarketEvent.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Series event is a snapshot of computed values that are available for all option series for
 * a given underlying symbol based on the option prices on the market.
 * It represents the most recent information that is available about the corresponding values on
 * the market at any given moment of time.
 *
 * <p>Series is an IndexedEvent with multiple instances of event available for
 * each underlying symbol. Each series event instance corresponds to an OptionSeries
 * of the corresponding underlying. The correspondence between a series event instance and
 * an OptionSeries is established via @ref ::getExpiration() "expiration" property.
 * If case where there are multiple series at the same expiration day id, then series events are
 * are ordered by their @ref #getIndex() "index" in the same order as the corresponding
 * OptionSeries are @ref OptionSeries::compareTo(OptionSeries) "ordered" by their attributes.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Series data source provides a consistent view of the set of known series.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for series, thus
 * @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p>IndexedEventModel class handles all the snapshot and transaction logic and conveniently represents
 * a list current of events.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `Series`.
 */
class DXFCPP_EXPORT Series final : public MarketEvent, public IndexedEvent {
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
        std::int32_t expiration{};
        double volatility = math::NaN;
        double callVolume = math::NaN;
        double putVolume = math::NaN;
        double putCallRatio = math::NaN;
        double forwardPrice = math::NaN;
        double dividend = math::NaN;
        double interest = math::NaN;
    };

    Data data_{};

    static std::shared_ptr<Series> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new series event with default values.
    Series() noexcept = default;

    /**
     * Creates new series event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Series(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

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
     * Returns time of this series event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this series event.
     */
    std::int64_t getTime() const {
        return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
               andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of this series event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this series event.
     */
    void setTime(std::int64_t time) {
        data_.timeSequence = orOp(
            sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
            orOp(sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_MASK), getSequence()));
    }

    /**
     * Returns sequence number of this series event to distinguish trades that have the same
     * @ref ::getTime() "time". This sequence number does not have to be unique and
     * does not need to be sequential. Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return sequence of this series event.
     */
    std::int32_t getSequence() const { return static_cast<std::int32_t>(andOp(data_.timeSequence, MAX_SEQUENCE)); }

    /**
     * Changes @ref ::getSequence() "sequence number" of this series event.
     *
     * @param sequence the sequence.
     * @see ::getSequence()
     */
    void setSequence(std::int32_t sequence) {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.timeSequence = orOp(andOp(data_.timeSequence, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns day id of expiration.
     *
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @return day id of expiration.
     */
    std::int32_t getExpiration() const { return data_.expiration; }

    /**
     * Changes day id of expiration.
     *
     * @param expiration day id of expiration.
     */
    void setExpiration(std::int32_t expiration) { data_.expiration = expiration; }

    /**
     * Returns implied volatility index for this series based on VIX methodology.
     *
     * @return implied volatility index for this series based on VIX methodology.
     */
    double getVolatility() const { return data_.volatility; }

    /**
     * Changes implied volatility index for this series based on VIX methodology.
     *
     * @param volatility implied volatility index for this series based on VIX methodology.
     */
    void setVolatility(double volatility) { data_.volatility = volatility; }

    /**
     * Returns call options traded volume for a day.
     *
     * @return call options traded volume for a day.
     */
    double getCallVolume() const { return data_.callVolume; }

    /**
     * Changes call options traded volume for a day.
     *
     * @param callVolume call options traded volume for a day.
     */
    void setCallVolume(double callVolume) { data_.callVolume = callVolume; }

    /**
     * Returns put options traded volume for a day.
     *
     * @return put options traded volume for a day.
     */
    double getPutVolume() const { return data_.putVolume; }

    /**
     * Changes put options traded volume for a day.
     *
     * @param putVolume put options traded volume for a day.
     */
    void setPutVolume(double putVolume) { data_.putVolume = putVolume; }

    /**
     * Returns options traded volume for a day.
     *
     * @return options traded volume for a day.
     */
    double getOptionVolume() const {
        if (std::isnan(data_.putVolume)) {
            return data_.callVolume;
        }

        if (std::isnan(data_.callVolume)) {
            return data_.putVolume;
        }

        return data_.putVolume + data_.callVolume;
    }

    /**
     * Returns ratio of put options traded volume to call options traded volume for a day.
     *
     * @return ratio of put options traded volume to call options traded volume for a day.
     */
    double getPutCallRatio() const { return data_.putCallRatio; }

    /**
     * Changes ratio of put options traded volume to call options traded volume for a day.
     *
     * @param putCallRatio ratio of put options traded volume to call options traded volume for a day.
     */
    void setPutCallRatio(double putCallRatio) { data_.putCallRatio = putCallRatio; }

    /**
     * Returns implied forward price for this option series.
     *
     * @return implied forward price for this option series.
     */
    double getForwardPrice() const { return data_.forwardPrice; }

    /**
     * Changes implied forward price for this option series.
     *
     * @param forwardPrice implied forward price for this option series.
     */
    void setForwardPrice(double forwardPrice) { data_.forwardPrice = forwardPrice; }

    /**
     * Returns implied simple dividend return of the corresponding option series.
     *
     * @return implied simple dividend return of the corresponding option series.
     */
    double getDividend() const { return data_.dividend; }

    /**
     * Changes implied simple dividend return of the corresponding option series.
     *
     * @param dividend implied simple dividend return of the corresponding option series.
     */
    void setDividend(double dividend) { data_.dividend = dividend; }

    /**
     * Returns implied simple interest return of the corresponding option series.
     *
     * @return implied simple interest return of the corresponding option series.
     */
    double getInterest() const { return data_.interest; }

    /**
     * Changes implied simple interest return of the corresponding option series.
     *
     * @param interest implied simple interest return of the corresponding option series.
     */
    void setInterest(double interest) { data_.interest = interest; }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp