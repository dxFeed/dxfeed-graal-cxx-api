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
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Quote event is a snapshot of the best bid and ask prices, and other fields that change with each quote.
 * It represents the most recent information that is available about the best quote on the market
 * at any given moment of time.
 *
 * Bid corresponds to the best (maximal price) order to buy,
 * ask corresponds to the best (minimal price) order to sell.
 */
class DXFCPP_EXPORT Quote final : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;

    struct Data {
        std::int32_t timeMillisSequence{};
        std::int32_t timeNanoPart{};
        std::int64_t bidTime{};
        std::int16_t bidExchangeCode{};
        double bidPrice = math::NaN;
        double bidSize = math::NaN;
        std::int64_t askTime{};
        std::int16_t askExchangeCode{};
        double askPrice = math::NaN;
        double askSize = math::NaN;
    };

    Data data_{};

    void recomputeTimeMillisPart() noexcept {
        data_.timeMillisSequence =
            orOp(sal(time_util::getMillisFromTime(std::max(data_.askTime, data_.bidTime)), MILLISECONDS_SHIFT),
                 getSequence());
    }

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the Quote object
    using Ptr = std::shared_ptr<Quote>;

    /// The alias to a type of unique pointer to the Quote object
    using Unique = std::unique_ptr<Quote>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see Quote::setSequence()
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

    /// Creates new quote event with default values.
    Quote() noexcept = default;

    /**
     * Creates new quote event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Quote(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

    /**
     * Changes event's symbol and returns the current quote.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current quote.
     */
    Quote &withEventSymbol(const std::string &eventSymbol) noexcept {
        MarketEvent::setEventSymbol(eventSymbol);

        return *this;
    }

    /**
     * Changes event's creation time and returns the current quote.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current quote.
     */
    Quote &withEventTime(std::int64_t eventTime) noexcept {
        MarketEvent::setEventTime(eventTime);

        return *this;
    }

    /**
     * Returns sequence number of this quote to distinguish quotes that have the same @ref Quote::getTime() "time". This
     * sequence number does not have to be unique and does not need to be sequential. Sequence can range from 0 to
     * Quote::MAX_SEQUENCE.
     *
     * @return sequence of this quote.
     */
    std::int32_t getSequence() const noexcept {
        return andOp(data_.timeMillisSequence, MAX_SEQUENCE);
    }

    /**
     * Changes @ref Quote::getSequence() "sequence number" of this quote.
     *
     * @param sequence The sequence.
     *
     * @see Quote::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence) {
        assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
        }

        data_.timeMillisSequence = orOp(andOp(data_.timeMillisSequence, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Changes @ref Quote::getSequence() "sequence number" of this quote and returns the current quote
     *
     * @param sequence The sequence.
     * @return The current quote.
     * @see Quote::getSequence()
     */
    Quote &withSequence(std::int32_t sequence) noexcept {
        setSequence(sequence);

        return *this;
    }

    /**
     * Returns time of the last bid or ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return The time of the last bid or ask change.
     */
    std::int64_t getTime() const noexcept {
        return math::floorDiv(std::max(data_.bidTime, data_.askTime), 1000LL) * 1000LL +
               shr(data_.timeMillisSequence, MILLISECONDS_SHIFT);
    }

    /**
     * Returns time of the last bid or ask change in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return The time of the last bid or ask change in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Returns microseconds and nanoseconds part of time of the last bid or ask change.
     *
     * @return The microseconds and nanoseconds part of time of the last bid or ask change.
     */
    std::int32_t getTimeNanoPart() const noexcept {
        return data_.timeNanoPart;
    }

    /**
     * Changes microseconds and nanoseconds part of time of the last bid or ask change.
     * <b>This method changes Quote::getTimeNanos() result.</b>
     *
     * @param timeNanoPart The microseconds and nanoseconds part of time of the last bid or ask change.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        data_.timeNanoPart = timeNanoPart;
    }

    /**
     * Changes microseconds and nanoseconds part of time of the last bid or ask change and returns the current quote.
     * <b>This method changes Quote::getTimeNanos() result.</b>
     *
     * @param timeNanoPart The microseconds and nanoseconds part of time of the last bid or ask change.
     * @return The current quote.
     */
    Quote &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        setTimeNanoPart(timeNanoPart);

        return *this;
    }

    /**
     * Returns time of the last bid change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return The time of the last bid change.
     */
    std::int64_t getBidTime() const noexcept {
        return data_.bidTime;
    }

    /**
     * Changes time of the last bid change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the Quote::getTime() of this quote even precise up to a millisecond.
     *
     * @param bidTime The time of the last bid change.
     */
    void setBidTime(std::int64_t bidTime) noexcept {
        data_.bidTime = bidTime;

        recomputeTimeMillisPart();
    }

    /**
     * Changes time of the last bid change and returns the current quote.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the Quote::getTime() of this quote even precise up to a millisecond.
     *
     * @param bidTime The time of the last bid change.
     * @return The current quote.
     */
    Quote &withBidTime(std::int64_t bidTime) noexcept {
        setBidTime(bidTime);

        return *this;
    }

    /**
     * Returns bid exchange code.
     *
     * @return The bid exchange code.
     */
    std::int16_t getBidExchangeCode() const noexcept;

    /**
     * Returns bid exchange code as UTF8 string.
     *
     * @return The bid exchange code as UTF8 string.
     */
    std::string getBidExchangeCodeString() const noexcept;

    /**
     * Changes bid exchange code.
     *
     * @param bidExchangeCode The bid exchange code.
     */
    void setBidExchangeCode(char bidExchangeCode) noexcept;

    /**
     * Changes bid exchange code and returns the current quote.
     *
     * @param bidExchangeCode The bid exchange code.
     * @return The current quote
     */
    Quote &withBidExchangeCode(char bidExchangeCode) noexcept {
        setBidExchangeCode(bidExchangeCode);

        return *this;
    }

    /**
     * Changes bid exchange code.
     *
     * @param bidExchangeCode bid exchange code.
     */
    void setBidExchangeCode(std::int16_t bidExchangeCode) noexcept;

    /**
     * Changes bid exchange code and returns the current quote.
     *
     * @param bidExchangeCode The bid exchange code.
     * @return The current quote.
     */
    Quote &withBidExchangeCode(std::int16_t bidExchangeCode) noexcept {
        setBidExchangeCode(bidExchangeCode);

        return *this;
    }

    /**
     * Returns bid price.
     *
     * @return The bid price.
     */
    double getBidPrice() const noexcept {
        return data_.bidPrice;
    }

    /**
     * Changes bid price.
     *
     * @param bidPrice The bid price.
     */
    void setBidPrice(double bidPrice) noexcept {
        data_.bidPrice = bidPrice;
    }

    /**
     * Changes bid price and returns the current quote.
     *
     * @param bidPrice The bid price.
     * @return The current quote
     */
    Quote &withBidPrice(double bidPrice) noexcept {
        setBidPrice(bidPrice);

        return *this;
    }

    /**
     * Returns bid size.
     *
     * @return The bid size
     */
    double getBidSize() const noexcept {
        return data_.bidSize;
    }

    /**
     * Changes bid size.
     *
     * @param bidSize The bid size.
     */
    void setBidSize(double bidSize) noexcept {
        data_.bidSize = bidSize;
    }

    /**
     * Changes bid size and returns the current quote.
     *
     * @param bidSize The bid size.
     * @return The current quote.
     */
    Quote &withBidSize(double bidSize) noexcept {
        setBidSize(bidSize);

        return *this;
    }

    /**
     * Returns time of the last ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return The time of the last ask change.
     */
    std::int64_t getAskTime() const noexcept {
        return data_.askTime;
    }

    /**
     * Changes time of the last ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the Quote::getTime() of this quote even precise up to a millisecond.
     *
     * @param askTime The time of the last ask change.
     */
    void setAskTime(std::int64_t askTime) noexcept {
        data_.askTime = askTime;

        recomputeTimeMillisPart();
    }

    /**
     * Changes time of the last ask change and returns the current quote.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the Quote::getTime() of this quote even precise up to a millisecond.
     *
     * @param askTime The time of the last ask change.
     * @return The current quote.
     */
    Quote &withAskTime(std::int64_t askTime) noexcept {
        setAskTime(askTime);

        return *this;
    }

    /**
     * Returns ask exchange code.
     *
     * @return The ask exchange code.
     */
    std::int16_t getAskExchangeCode() const noexcept;

    /**
     * Returns ask exchange code as UTF8 string.
     *
     * @return The ask exchange code as UTF8 string.
     */
    std::string getAskExchangeCodeString() const noexcept;

    /**
     * Changes ask exchange code.
     *
     * @param askExchangeCode The ask exchange code.
     */
    void setAskExchangeCode(char askExchangeCode) noexcept;

    /**
     * Changes ask exchange code and returns the current quote.
     *
     * @param askExchangeCode The ask exchange code.
     * @return The current quote
     */
    Quote &withAskExchangeCode(char askExchangeCode) noexcept {
        setAskExchangeCode(askExchangeCode);

        return *this;
    }

    /**
     * Changes ask exchange code.
     *
     * @param askExchangeCode The ask exchange code.
     */
    void setAskExchangeCode(std::int16_t askExchangeCode) noexcept;

    /**
     * Changes ask exchange code and returns the current quote.
     *
     * @param askExchangeCode The ask exchange code.
     * @return The current quote.
     */
    Quote &withAskExchangeCode(std::int16_t askExchangeCode) noexcept {
        setAskExchangeCode(askExchangeCode);

        return *this;
    }

    /**
     * Returns ask price.
     *
     * @return The ask price.
     */
    double getAskPrice() const {
        return data_.askPrice;
    }

    /**
     * Changes ask price.
     *
     * @param askPrice The ask price.
     */
    void setAskPrice(double askPrice) {
        data_.askPrice = askPrice;
    }

    /**
     * Changes ask price and returns the current quote.
     *
     * @param askPrice The ask price.
     * @return The current quote
     */
    Quote &withAskPrice(double askPrice) noexcept {
        setAskPrice(askPrice);

        return *this;
    }

    /**
     * Returns ask size.
     *
     * @return The ask size
     */
    double getAskSize() const {
        return data_.askSize;
    }

    /**
     * Changes ask size.
     *
     * @param askSize The ask size.
     */
    void setAskSize(double askSize) {
        data_.askSize = askSize;
    }

    /**
     * Changes ask size and returns the current quote.
     *
     * @param askSize The ask size.
     * @return The current quote.
     */
    Quote &withAskSize(double askSize) noexcept {
        setAskSize(askSize);

        return *this;
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation.
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()