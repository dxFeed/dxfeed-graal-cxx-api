// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "../market/MarketEvent.hpp"

namespace dxfcpp {

struct EventMapper;

// TODO: implement

class Underlying : public MarketEvent, public TimeSeriesEvent, public LastingEvent {
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

    // ========================= instance =========================

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
        double volatility = detail::math::NaN;
        double frontVolatility = detail::math::NaN;
        double backVolatility = detail::math::NaN;
        double callVolume = detail::math::NaN;
        double putVolume = detail::math::NaN;
        double putCallRatio = detail::math::NaN;
    };

    Data data_{};

    static std::shared_ptr<Underlying> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &Type;

    /// Creates new greeks with default values.
    Underlying() noexcept = default;

    /**
     * Creates new greeks with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Underlying(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

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
        return detail::BitOps::sar(data_.index, SECONDS_SHIFT) * 1000 +
               detail::BitOps::andOp(detail::BitOps::sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes timestamp of the event in milliseconds.
     *
     * @param time timestamp of the event in milliseconds.
     * @see ::getTime()
     */
    void setTime(std::int64_t time) {
        data_.index = detail::BitOps::orOp(
            detail::BitOps::orOp(detail::BitOps::sal(detail::time_util::getSecondsFromTime(time), SECONDS_SHIFT),
                                 detail::BitOps::sal(detail::time_util::getMillisFromTime(time), MILLISECONDS_SHIFT)),
            getSequence());
    }

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const {
        return static_cast<std::int32_t>(detail::BitOps::andOp(data_.index, MAX_SEQUENCE));
    }

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     * @param sequence the sequence.
     * @see ::getSequence()
     */
    void setSequence(int sequence) {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.index = detail::BitOps::orOp(detail::BitOps::andOp(data_.index, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns 30-day implied volatility for this underlying based on VIX methodology.
     *
     * @return 30-day implied volatility for this underlying based on VIX methodology.
     */
    double getVolatility() const { return data_.volatility; }

    /**
     * Changes 30-day implied volatility for this underlying based on VIX methodology.
     *
     * @param volatility 30-day implied volatility for this underlying based on VIX methodology.
     */
    void setVolatility(double volatility) { data_.volatility = volatility; }

    /**
     * Returns front month implied volatility for this underlying based on VIX methodology.
     *
     * @return front month implied volatility for this underlying based on VIX methodology.
     */
    double getFrontVolatility() const { return data_.frontVolatility; }

    /**
     * Changes front month implied volatility for this underlying based on VIX methodology.
     *
     * @param frontVolatility front month implied volatility for this underlying based on VIX methodology.
     */
    void setFrontVolatility(double frontVolatility) { data_.frontVolatility = frontVolatility; }

    /**
     * Returns back month implied volatility for this underlying based on VIX methodology.
     *
     * @return back month implied volatility for this underlying based on VIX methodology.
     */
    double getBackVolatility() const { return data_.backVolatility; }

    /**
     * Changes back month implied volatility for this underlying based on VIX methodology.
     *
     * @param backVolatility back month implied volatility for this underlying based on VIX methodology.
     */
    void setBackVolatility(double backVolatility) { data_.backVolatility = backVolatility; }

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
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept {
        return fmt::format(
            "Greeks{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, volatility={}, frontVolatility={}, "
            "backVolatility={}, callVolume={}, putVolume={}, putCallRatio={}}}",
            MarketEvent::getEventSymbol(), detail::formatTimeStampWithMillis(MarketEvent::getEventTime()),
            getEventFlags().getMask(), detail::formatTimeStampWithMillis(getTime()), getSequence(), getVolatility(),
            getFrontVolatility(), getBackVolatility(), getCallVolume(), getPutVolume(), getPutCallRatio());
    }

    template <typename OStream> friend OStream &operator<<(OStream &os, const Underlying &e) {
        return os << e.toString();
    }

    template <typename OStream> friend OStream &operator<<(OStream &os, const std::shared_ptr<Underlying> &e) {
        return os << e->toString();
    }
};

} // namespace dxfcpp