// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

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

namespace dxfcpp {

struct EventMapper;

// TODO: doc
class TheoPrice : public MarketEvent, public TimeSeriesEvent, public LastingEvent {
    friend struct EventMapper;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static constexpr std::int32_t MAX_SEQUENCE = (1 << 22) - 1;
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

    static std::shared_ptr<TheoPrice> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &Type;

    /// Creates new theoprice with default values.
    TheoPrice() noexcept = default;

    /**
     * Creates new theoprice with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TheoPrice(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

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
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const { return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE)); }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     * @param sequence the sequence.
     * @see ::getSequence()
     */
    void setSequence(int sequence) {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns theoretical option price.
     *
     * @return theoretical option price.
     */
    double getPrice() const { return data_.price; }

    /**
     * Changes theoretical option price.
     *
     * @param price theoretical option price.
     */
    void setPrice(double price) { data_.price = price; }

    /**
     * Returns underlying price at the time of theo price computation.
     *
     * @return underlying price at the time of theo price computation.
     */
    double getUnderlyingPrice() const { return data_.underlyingPrice; }

    /**
     * Changes underlying price at the time of theo price computation.
     *
     * @param underlyingPrice underlying price at the time of theo price computation.
     */
    void setUnderlyingPrice(double underlyingPrice) { data_.underlyingPrice = underlyingPrice; }

    /**
     * Returns delta of the theoretical price.
     * Delta is the first derivative of the theoretical price by the underlying price.
     *
     * @return delta of the theoretical price.
     */
    double getDelta() const { return data_.delta; }

    /**
     * Changes delta of the theoretical price.
     *
     * @param delta delta of the theoretical price.
     */
    void setDelta(double delta) { data_.delta = delta; }

    /**
     * Returns gamma of the theoretical price.
     * Gamma is the second derivative of the theoretical price by the underlying price.
     *
     * @return gamma of the theoretical price.
     */
    double getGamma() const { return data_.gamma; }

    /**
     * Changes gamma of the theoretical price.
     *
     * @param gamma gamma of the theoretical price.
     */
    void setGamma(double gamma) { data_.gamma = gamma; }

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
    std::string toString() const noexcept {
        return fmt::format(
            "TheoPrice{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, underlyingPrice={}, "
            "delta={}, gamma={}, dividend={}, interest={}}}",
            MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
            getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getSequence(), getPrice(),
            getUnderlyingPrice(), getDelta(), getGamma(), getDividend(), getInterest());
    }

    template <typename OStream> friend OStream &operator<<(OStream &os, const TheoPrice &e) {
        return os << e.toString();
    }

    template <typename OStream> friend OStream &operator<<(OStream &os, const std::shared_ptr<TheoPrice> &e) {
        return os << e->toString();
    }
};

} // namespace dxfcpp