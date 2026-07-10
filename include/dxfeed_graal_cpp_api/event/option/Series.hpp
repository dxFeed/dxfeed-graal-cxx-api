// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEvent.hpp"
#include "../IndexedEventSource.hpp"
#include "../market/MarketEvent.hpp"

#include <cstdint>
#include <memory>
#include <string>

/**
 * \addtogroup dxfcpp_option
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Series event is a snapshot of computed values that are available for all option series for
 * a given underlying symbol based on the option prices on the market.
 * It represents the most recent information that is available about the corresponding values on
 * the market at any given moment of time.
 *
 * <p>Series is an IndexedEvent with multiple instances of an event available for
 * each underlying symbol. Each series event instance corresponds to an OptionSeries
 * of the corresponding underlying. The correspondence between a series event instance and
 * an OptionSeries is established via @ref Series::getExpiration() "expiration" property.
 * If case where there are multiple series at the same expiration day id, then series events are
 * are ordered by their @ref Series::getIndex() "index" in the same order as the corresponding
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

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    public:
    /// The alias to a type of shared pointer to the Series object
    using Ptr = std::shared_ptr<Series>;

    /// The alias to a type of unique pointer to the Series object
    using Unique = std::unique_ptr<Series>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
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

    /// Creates new series event with default values.
    Series() noexcept;

    /**
     * Creates a new series event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Series(const StringLike & eventSymbol) noexcept;

    /**
     * Changes an event's symbol and returns the current series.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current series.
     */
    Series &withEventSymbol(const StringLike &eventSymbol) noexcept;

    /**
     * Changes event's creation time and returns the current series.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current series.
     */
    Series &withEventTime(std::int64_t eventTime) noexcept;

    ///
    const IndexedEventSource &getSource() const & noexcept override {
        return IndexedEventSource::DEFAULT;
    }

    ///
    std::int32_t getEventFlags() const noexcept override;

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override;

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override;

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current series.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current series.
     */
    Series &withEventFlags(std::int32_t eventFlags) noexcept;

    /**
     * Changes transactional event flags and returns the current series.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current series.
     */
    Series &withEventFlags(const EventFlagsMask &eventFlags) noexcept;

    /**
     * Returns a unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence".
     * Changing either time or sequence changes the event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override;

    /**
     * Changes the unique per-symbol index of this event.
     *
     * @param index the event index.
     * @see Series::getIndex()
     */
    void setIndex(std::int64_t index) override;

    /**
     * Changes the unique per-symbol index of this series and returns it.
     *
     * @param index unique per-symbol index of this series.
     * @return The current series.
     */
    Series &withIndex(std::int64_t index) noexcept;

    /**
     * Returns time and sequence of this event packaged into a single long value.
     *
     * @return time and sequence of this event.
     */
    std::int64_t getTimeSequence() const noexcept;

    /**
     * Changes time and sequence of this event.
     * <b>Do not use this method directly.</b>
     * Change @ref Series::setTime() "time" and/or @ref Series::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Returns time of this series event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of this series event.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Changes time of this series event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this series event.
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Changes time of this series and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this series.
     * @return The current series.
     */
    Series &withTime(std::int64_t time) noexcept;

    /**
     * Returns sequence number of this series event to distinguish trades that have the same
     * @ref ::getTime() "time". This sequence number does not have to be unique and
     * does not need to be sequential. Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return sequence of this series event.
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes @ref Series::getSequence() "sequence number" of this series event.
     *
     * @param sequence the sequence.
     * @see Series::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence);

    /**
     * Changes @ref Series::getSequence() "sequence number" of this series.
     * Returns the current order.
     *
     * @param sequence the sequence.
     * @return The current series.
     * @see Series::getSequence()
     */
    Series &withSequence(std::int32_t sequence) noexcept;

    /**
     * Returns day id of expiration.
     *
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @return day id of expiration.
     */
    std::int32_t getExpiration() const noexcept;

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @param expiration The day id of expiration.
     */
    void setExpiration(std::int32_t expiration) noexcept;

    /**
     * Returns implied volatility index for this series based on VIX methodology.
     *
     * @return implied volatility index for this series based on VIX methodology.
     */
    double getVolatility() const noexcept;

    /**
     * Changes implied volatility index for this series based on VIX methodology.
     *
     * @param volatility implied volatility index for this series based on VIX methodology.
     */
    void setVolatility(double volatility) noexcept;

    /**
     * Returns call options traded volume for a day.
     *
     * @return call options traded volume for a day.
     */
    double getCallVolume() const noexcept;

    /**
     * Changes call options traded volume for a day.
     *
     * @param callVolume call options traded volume for a day.
     */
    void setCallVolume(double callVolume) noexcept;

    /**
     * Returns put options traded volume for a day.
     *
     * @return put options traded volume for a day.
     */
    double getPutVolume() const noexcept;

    /**
     * Changes put options traded volume for a day.
     *
     * @param putVolume put options traded volume for a day.
     */
    void setPutVolume(double putVolume) noexcept;

    /**
     * Returns options traded volume for a day.
     *
     * @return options traded volume for a day.
     */
    double getOptionVolume() const noexcept;

    /**
     * Returns ratio of put options traded volume to call options traded volume for a day.
     *
     * @return ratio of put options traded volume to call options traded volume for a day.
     */
    double getPutCallRatio() const noexcept;

    /**
     * Changes ratio of put options traded volume to call options traded volume for a day.
     *
     * @param putCallRatio ratio of put options traded volume to call options traded volume for a day.
     */
    void setPutCallRatio(double putCallRatio) noexcept;

    /**
     * Returns implied forward price for this option series.
     *
     * @return implied forward price for this option series.
     */
    double getForwardPrice() const noexcept;

    /**
     * Changes implied forward price for this option series.
     *
     * @param forwardPrice implied forward price for this option series.
     */
    void setForwardPrice(double forwardPrice) noexcept;

    /**
     * Returns implied simple dividend return of the corresponding option series.
     *
     * @return implied simple dividend return of the corresponding option series.
     */
    double getDividend() const noexcept;

    /**
     * Changes implied simple dividend return of the corresponding option series.
     *
     * @param dividend implied simple dividend return of the corresponding option series.
     */
    void setDividend(double dividend) noexcept;

    /**
     * Returns implied simple interest return of the corresponding option series.
     *
     * @return implied simple interest return of the corresponding option series.
     */
    double getInterest() const noexcept;

    /**
     * Changes implied simple interest return of the corresponding option series.
     *
     * @param interest implied simple interest return of the corresponding option series.
     */
    void setInterest(double interest) noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()