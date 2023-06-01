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
 * Base class for common fields of Trade} and TradeETH events.
 * Trade events represent the most recent information that is available about the last trade on the market at any
 * given moment of time.
 *
 * Trade event represents last trade information for <b>regular trading hours</b> (RTH) with an official volume and
 * turnover <b>for the whole trading day</b> identified by @ref ::getDayId() "dayId" field.
 *
 * TradeETH event is defined only for symbols (typically stocks and ETFs) with a designated
 * <b>extended trading hours</b> (ETH, pre market and post market trading sessions). It represents last trade price
 * during ETH and also accumulated volume and turnover during ETH for current trading day.
 */
class DXFCPP_EXPORT TradeBase : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

  protected:
    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..4     3    2    1    0
     * +--------+----+----+----+----+
     * |        |  Direction   | ETH|
     * +--------+----+----+----+----+
     */

    // DIRECTION values are taken from Direction enum.
    static constexpr std::int32_t DIRECTION_MASK = 7;
    static constexpr std::int32_t DIRECTION_SHIFT = 1;

    static constexpr std::int32_t ETH = 1;

    struct Data {
        std::int64_t timeSequence{};
        std::int32_t timeNanoPart{};
        std::int16_t exchangeCode{};
        double price = math::NaN;
        double change = math::NaN;
        double size = math::NaN;
        std::int32_t dayId{};
        double dayVolume{};
        double dayTurnover{};
        std::int32_t flags{};
    };

    Data data_{};

    template <typename ChildType, typename GraalNativeEventType, typename ChildGraalNativeEventType, auto clazz>
    static std::shared_ptr<ChildType> fromGraalNative(void *graalNative) noexcept
#if __cpp_concepts
        requires(std::is_base_of_v<TradeBase, ChildType>)
#endif
    {
        if (!graalNative) {
            return {};
        }

        if (bit_cast<GraalNativeEventType *>(graalNative)->clazz != clazz) {
            return {};
        }

        try {
            auto graalTrade = bit_cast<ChildGraalNativeEventType *>(graalNative);
            auto trade =
                std::make_shared<ChildType>(dxfcpp::toString(graalTrade->trade_base.market_event.event_symbol));

            trade->setEventTime(graalTrade->trade_base.market_event.event_time);
            trade->data_ = {
                graalTrade->trade_base.time_sequence, graalTrade->trade_base.time_nano_part,
                graalTrade->trade_base.exchange_code, graalTrade->trade_base.price,
                graalTrade->trade_base.change,        graalTrade->trade_base.size,
                graalTrade->trade_base.day_id,        graalTrade->trade_base.day_volume,
                graalTrade->trade_base.day_turnover,  graalTrade->trade_base.flags,
            };

            return trade;
        } catch (...) {
            // TODO: error handling
            return {};
        }
    }

  public:
    /// Creates new trade event with default values.
    TradeBase() noexcept = default;

    /**
     * Creates new trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TradeBase(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

    /**
     * Returns time and sequence of last trade packaged into single long value.
     *
     * @return time and sequence of last trade.
     */
    std::int64_t getTimeSequence() const { return data_.timeSequence; }

    /**
     * Changes time and sequence of last trade.
     * <b>Do not use this method directly.</b>
     * Change @ref ::setTime() "time" and/or @ref ::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) { data_.timeSequence = timeSequence; }

    /**
     * Returns time of the last trade.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last trade.
     */
    std::int64_t getTime() const {
        return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
               andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of the last trade.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of the last trade.
     */
    void setTime(std::int64_t time) {
        data_.timeSequence = orOp(
            sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
            orOp(sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_MASK), getSequence()));
    }

    /**
     * Returns time of the last trade in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last trade in nanoseconds.
     */
    std::int64_t getTimeNanos() const {
        return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Changes time of the last trade.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the last trade in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) {
        setTime(time_nanos_util::getMillisFromNanos(timeNanos));
        data_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
    }

    /**
     * Changes microseconds and nanoseconds time part of the last trade.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the last trade.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) { data_.timeNanoPart = timeNanoPart; }

    /**
     * Returns microseconds and nanoseconds time part of the last trade.
     *
     * @return microseconds and nanoseconds time part of the last trade.
     */
    std::int32_t getTimeNanoPart() const { return data_.timeNanoPart; }

    /**
     * Returns sequence number of the last trade to distinguish trades that have the same
     * @ref ::getTime() "time". This sequence number does not have to be unique and
     * does not need to be sequential. Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return sequence of the last trade.
     */
    std::int32_t getSequence() const { return static_cast<std::int32_t>(andOp(data_.timeSequence, MAX_SEQUENCE)); }

    /**
     * Changes @ref ::getSequence() "sequence number" of the last trade.
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
     * Returns exchange code of the last trade.
     *
     * @return exchange code of the last trade.
     */
    std::int16_t getExchangeCode() const { return data_.exchangeCode; }

    /**
     * Changes exchange code of the last trade.
     *
     * @param exchangeCode exchange code of the last trade.
     */
    void setExchangeCode(char exchangeCode);

    /**
     * Changes exchange code of the last trade.
     *
     * @param exchangeCode exchange code of the last trade.
     */
    void setExchangeCode(std::int16_t exchangeCode) { data_.exchangeCode = exchangeCode; }

    /**
     * Returns price of the last trade.
     *
     * @return price of the last trade.
     */
    double getPrice() const { return data_.price; }

    /**
     * Changes price of the last trade.
     *
     * @param price price of the last trade.
     */
    void setPrice(double price) { data_.price = price; }

    /**
     * Returns size of the last trade as floating number with fractions.
     *
     * @return size of the last trade as floating number with fractions.
     */
    double getSize() const { return data_.size; }

    /**
     * Changes size of the last trade as floating number with fractions.
     *
     * @param size size of the last trade as floating number with fractions.
     */
    void setSize(double size) { data_.size = size; }

    /**
     * Returns identifier of the current trading day.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return identifier of the current trading day.
     */
    std::int32_t getDayId() const { return data_.dayId; }

    /**
     * Changes identifier of the current trading day.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param dayId identifier of the current trading day.
     */
    void setDayId(std::int32_t dayId) { data_.dayId = dayId; }

    /**
     * Returns total volume traded for a day as floating number with fractions.
     *
     * @return total volume traded for a day as floating number with fractions.
     */
    double getDayVolume() const { return data_.dayVolume; }

    /**
     * Changes total volume traded for a day as floating number with fractions.
     *
     * @param dayVolume total volume traded for a day as floating number with fractions.
     */
    void setDayVolume(double dayVolume) { data_.dayVolume = dayVolume; }

    /**
     * Returns total turnover traded for a day.
     * Day VWAP can be computed with `getDayTurnover() / getDayVolume()`.
     *
     * @return total turnover traded for a day.
     */
    double getDayTurnover() const { return data_.dayTurnover; }

    /**
     * Changes total turnover traded for a day.
     *
     * @param dayTurnover total turnover traded for a day.
     */
    void setDayTurnover(double dayTurnover) { data_.dayTurnover = dayTurnover; }

    /**
     * Returns tick direction of the last trade.
     *
     * @return tick direction of the last trade.
     */
    const Direction &getTickDirection() const & {
        return Direction::valueOf(getBits(data_.flags, DIRECTION_MASK, DIRECTION_SHIFT));
    }

    /**
     * Changes tick direction of the last trade.
     *
     * @param direction tick direction of the last trade.
     */
    void setTickDirection(const Direction &direction) {
        data_.flags = setBits(data_.flags, DIRECTION_MASK, DIRECTION_SHIFT, direction.getCode());
    }

    /**
     * Returns whether last trade was in extended trading hours.
     *
     * @return `true` if last trade was in extended trading hours.
     */
    bool isExtendedTradingHours() const { return andOp(data_.flags, ETH) != 0; }

    /**
     * Changes whether last trade was in extended trading hours.
     *
     * @param extendedTradingHours `true` if last trade was in extended trading hours.
     */
    void setExtendedTradingHours(bool extendedTradingHours) {
        data_.flags = extendedTradingHours ? orOp(data_.flags, ETH) : andOp(data_.flags, ~ETH);
    }

    /**
     * Returns change of the last trade.
     *
     * @return change of the last trade.
     */
    double getChange() const { return data_.change; }

    /**
     * Changes change of the last trade.
     *
     * @param change price of the last trade.
     */
    void setChange(double change) { data_.change = change; }

    /**
     * Returns string representation of this trade event's fields.
     *
     * @return string representation of this trade event's fields.
     */
    std::string baseFieldsToString() const noexcept;
};

} // namespace dxfcpp