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
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "../market/MarketEvent.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Underlying event is a snapshot of computed values that are available for an option underlying
 * symbol based on the option prices on the market.
 * It represents the most recent information that is available about the corresponding values on
 * the market at any given moment of time.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some Underlying sources provide a consistent view of the set of known Underlying events.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for Underlying, thus
 * @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p>TimeSeriesEventModel class handles all the snapshot and transaction logic and conveniently represents
 * a list current of time-series events order by their @ref ::getTime() "time".
 *
 * <h3>Publishing Underlying</h3>
 *
 * Publishing of Underlying events follows the general rules explained in TimeSeriesEvent class
 * documentation.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `Underlying`.
 */
class DXFCPP_EXPORT Underlying final : public MarketEvent, public TimeSeriesEvent, public LastingEvent {
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
        double volatility = math::NaN;
        double frontVolatility = math::NaN;
        double backVolatility = math::NaN;
        double callVolume = math::NaN;
        double putVolume = math::NaN;
        double putCallRatio = math::NaN;
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the Underlying object
    using Ptr = std::shared_ptr<Underlying>;

    /// The alias to a type of unique pointer to the Underlying object
    using Unique = std::unique_ptr<Underlying>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
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

    /// Creates new underlying event with default values.
    Underlying() noexcept = default;

    /**
     * Creates new underlying event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Underlying(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

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

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override {
        data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
    }

    /**
     * Returns unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence".
     * Changing either time or sequence changes event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override {
        return data_.index;
    }

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
    void setIndex(std::int64_t index) override {
        data_.index = index;
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
     * @see ::getTime()
     */
    void setTime(std::int64_t time) noexcept {
        data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                                sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                           getSequence());
    }

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const noexcept {
        return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
    }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     * @param sequence the sequence.
     * @see ::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence) {
        assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
        }

        data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns 30-day implied volatility for this underlying based on VIX methodology.
     *
     * @return 30-day implied volatility for this underlying based on VIX methodology.
     */
    double getVolatility() const noexcept {
        return data_.volatility;
    }

    /**
     * Changes 30-day implied volatility for this underlying based on VIX methodology.
     *
     * @param volatility 30-day implied volatility for this underlying based on VIX methodology.
     */
    void setVolatility(double volatility) noexcept {
        data_.volatility = volatility;
    }

    /**
     * Returns front month implied volatility for this underlying based on VIX methodology.
     *
     * @return front month implied volatility for this underlying based on VIX methodology.
     */
    double getFrontVolatility() const noexcept {
        return data_.frontVolatility;
    }

    /**
     * Changes front month implied volatility for this underlying based on VIX methodology.
     *
     * @param frontVolatility front month implied volatility for this underlying based on VIX methodology.
     */
    void setFrontVolatility(double frontVolatility) noexcept {
        data_.frontVolatility = frontVolatility;
    }

    /**
     * Returns back month implied volatility for this underlying based on VIX methodology.
     *
     * @return back month implied volatility for this underlying based on VIX methodology.
     */
    double getBackVolatility() const noexcept {
        return data_.backVolatility;
    }

    /**
     * Changes back month implied volatility for this underlying based on VIX methodology.
     *
     * @param backVolatility back month implied volatility for this underlying based on VIX methodology.
     */
    void setBackVolatility(double backVolatility) noexcept {
        data_.backVolatility = backVolatility;
    }

    /**
     * Returns call options traded volume for a day.
     *
     * @return call options traded volume for a day.
     */
    double getCallVolume() const noexcept {
        return data_.callVolume;
    }

    /**
     * Changes call options traded volume for a day.
     *
     * @param callVolume call options traded volume for a day.
     */
    void setCallVolume(double callVolume) noexcept {
        data_.callVolume = callVolume;
    }

    /**
     * Returns put options traded volume for a day.
     *
     * @return put options traded volume for a day.
     */
    double getPutVolume() const noexcept {
        return data_.putVolume;
    }

    /**
     * Changes put options traded volume for a day.
     *
     * @param putVolume put options traded volume for a day.
     */
    void setPutVolume(double putVolume) noexcept {
        data_.putVolume = putVolume;
    }

    /**
     * Returns options traded volume for a day.
     *
     * @return options traded volume for a day.
     */
    double getOptionVolume() const noexcept {
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
    double getPutCallRatio() const noexcept {
        return data_.putCallRatio;
    }

    /**
     * Changes ratio of put options traded volume to call options traded volume for a day.
     *
     * @param putCallRatio ratio of put options traded volume to call options traded volume for a day.
     */
    void setPutCallRatio(double putCallRatio) noexcept {
        data_.putCallRatio = putCallRatio;
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