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
 * Theo price is a snapshot of the theoretical option price computation that is
 * periodically performed by <a href="http://www.devexperts.com/en/products/price.html">dxPrice</a>
 * model-free computation.
 * It represents the most recent information that is available about the corresponding
 * values at any given moment of time.
 * The values include first and second order derivative of the price curve by price, so that
 * the real-time theoretical option price can be estimated on real-time changes of the underlying
 * price in the vicinity.
 *
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some TheoPrice sources provide a consistent view of the set of known TheoPrice.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for TheoPrice, thus
 * @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p>TimeSeriesEventModel class handles all the snapshot and transaction logic and conveniently represents
 * a list current of time-series events order by their @ref ::getTime() "time".
 *
 * <h3>Publishing TheoPrice</h3>
 *
 * Publishing of TheoPrice events follows the general rules explained in TimeSeriesEvent class
 * documentation.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `TheoPrice`.
 */
class DXFCPP_EXPORT TheoPrice final : public MarketEvent, public TimeSeriesEvent, public LastingEvent {
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
        double price = math::NaN;
        double underlyingPrice = math::NaN;
        double delta = math::NaN;
        double gamma = math::NaN;
        double dividend = math::NaN;
        double interest = math::NaN;
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the TheoPrice object
    using Ptr = std::shared_ptr<TheoPrice>;

    /// The alias to a type of unique pointer to the TheoPrice object
    using Unique = std::unique_ptr<TheoPrice>;

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

    /// Creates new theoprice event with default values.
    TheoPrice() noexcept = default;

    /**
     * Creates new theoprice event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TheoPrice(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
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
     * Returns theoretical option price.
     *
     * @return theoretical option price.
     */
    double getPrice() const noexcept {
        return data_.price;
    }

    /**
     * Changes theoretical option price.
     *
     * @param price theoretical option price.
     */
    void setPrice(double price) noexcept {
        data_.price = price;
    }

    /**
     * Returns underlying price at the time of theo price computation.
     *
     * @return underlying price at the time of theo price computation.
     */
    double getUnderlyingPrice() const noexcept {
        return data_.underlyingPrice;
    }

    /**
     * Changes underlying price at the time of theo price computation.
     *
     * @param underlyingPrice underlying price at the time of theo price computation.
     */
    void setUnderlyingPrice(double underlyingPrice) noexcept {
        data_.underlyingPrice = underlyingPrice;
    }

    /**
     * Returns delta of the theoretical price.
     * Delta is the first derivative of the theoretical price by the underlying price.
     *
     * @return delta of the theoretical price.
     */
    double getDelta() const noexcept {
        return data_.delta;
    }

    /**
     * Changes delta of the theoretical price.
     *
     * @param delta delta of the theoretical price.
     */
    void setDelta(double delta) noexcept {
        data_.delta = delta;
    }

    /**
     * Returns gamma of the theoretical price.
     * Gamma is the second derivative of the theoretical price by the underlying price.
     *
     * @return gamma of the theoretical price.
     */
    double getGamma() const noexcept {
        return data_.gamma;
    }

    /**
     * Changes gamma of the theoretical price.
     *
     * @param gamma gamma of the theoretical price.
     */
    void setGamma(double gamma) noexcept {
        data_.gamma = gamma;
    }

    /**
     * Returns implied simple dividend return of the corresponding option series.
     *
     * @return implied simple dividend return of the corresponding option series.
     */
    double getDividend() const noexcept {
        return data_.dividend;
    }

    /**
     * Changes implied simple dividend return of the corresponding option series.
     *
     * @param dividend implied simple dividend return of the corresponding option series.
     */
    void setDividend(double dividend) noexcept {
        data_.dividend = dividend;
    }

    /**
     * Returns implied simple interest return of the corresponding option series.
     *
     * @return implied simple interest return of the corresponding option series.
     */
    double getInterest() const noexcept {
        return data_.interest;
    }

    /**
     * Changes implied simple interest return of the corresponding option series.
     *
     * @param interest implied simple interest return of the corresponding option series.
     */
    void setInterest(double interest) noexcept {
        data_.interest = interest;
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