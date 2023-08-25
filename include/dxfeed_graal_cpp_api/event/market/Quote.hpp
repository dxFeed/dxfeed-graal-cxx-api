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
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"

namespace dxfcpp {

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

    static std::shared_ptr<Quote> fromGraal(void *graalNative) noexcept;

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
     * @see Quote::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

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
     */
    void setSequence(std::int32_t sequence) noexcept {
        // TODO: Improve error handling
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.timeMillisSequence = orOp(andOp(data_.timeMillisSequence, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Returns time of the last bid or ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid or ask change.
     */
    std::int64_t getTime() const noexcept {
        return math::floorDiv(std::max(data_.bidTime, data_.askTime), 1000LL) * 1000LL +
               shr(data_.timeMillisSequence, MILLISECONDS_SHIFT);
    }

    /**
     * Returns time of the last bid or ask change in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid or ask change in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Returns microseconds and nanoseconds part of time of the last bid or ask change.
     *
     * @return microseconds and nanoseconds part of time of the last bid or ask change.
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
    void setTimeNanoPart(int32_t timeNanoPart) noexcept {
        data_.timeNanoPart = timeNanoPart;
    }

    /**
     * Returns time of the last bid change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return time of the last bid change.
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
     * @param bidTime time of the last bid change.
     */
    void setBidTime(std::int64_t bidTime) noexcept {
        data_.bidTime = bidTime;

        recomputeTimeMillisPart();
    }

    /**
     * Returns bid exchange code.
     *
     * @return bid exchange code.
     */
    std::int16_t getBidExchangeCode() const noexcept;

    /**
     * Changes bid exchange code.
     *
     * @param bidExchangeCode bid exchange code.
     */
    void setBidExchangeCode(char bidExchangeCode) noexcept;

    /**
     * Changes bid exchange code.
     *
     * @param bidExchangeCode bid exchange code.
     */
    void setBidExchangeCode(std::int16_t bidExchangeCode) noexcept;

    /**
     * Returns bid price.
     *
     * @return bid price.
     */
    double getBidPrice() const noexcept {
        return data_.bidPrice;
    }

    /**
     * Changes bid price.
     *
     * @param bidPrice bid price.
     */
    void setBidPrice(double bidPrice) noexcept {
        data_.bidPrice = bidPrice;
    }

    /**
     * Returns bid size.
     *
     * @return bid size
     */
    double getBidSize() const noexcept {
        return data_.bidSize;
    }

    /**
     * Changes bid size.
     *
     * @param bidSize bid size.
     */
    void setBidSize(double bidSize) noexcept {
        data_.bidSize = bidSize;
    }

    /**
     * Returns time of the last ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return time of the last ask change.
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
     * @param askTime time of the last ask change.
     */
    void setAskTime(std::int64_t askTime) noexcept {
        data_.askTime = askTime;

        recomputeTimeMillisPart();
    }

    /**
     * Returns ask exchange code.
     *
     * @return ask exchange code.
     */
    std::int16_t getAskExchangeCode() const noexcept;

    /**
     * Changes ask exchange code.
     *
     * @param askExchangeCode ask exchange code.
     */
    void setAskExchangeCode(char askExchangeCode) noexcept;

    /**
     * Changes ask exchange code.
     *
     * @param askExchangeCode ask exchange code.
     */
    void setAskExchangeCode(std::int16_t askExchangeCode) noexcept;

    /**
     * Returns ask price.
     *
     * @return ask price.
     */
    double getAskPrice() const {
        return data_.askPrice;
    }

    /**
     * Changes ask price.
     *
     * @param askPrice ask price.
     */
    void setAskPrice(double askPrice) {
        data_.askPrice = askPrice;
    }

    /**
     * Returns ask size.
     *
     * @return ask size
     */
    double getAskSize() const {
        return data_.askSize;
    }

    /**
     * Changes ask size.
     *
     * @param askSize ask size.
     */
    void setAskSize(double askSize) {
        data_.askSize = askSize;
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp