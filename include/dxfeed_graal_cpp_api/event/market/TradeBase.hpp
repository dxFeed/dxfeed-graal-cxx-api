// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../LastingEvent.hpp"
#include "./MarketEvent.hpp"

#include <cstdint>
#include <memory>
#include <string>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct Direction;
struct EventMapper;

/**
 * Base class for common fields of Trade and TradeETH events.
 * Trade events represent the most recent information that is available about the last trade on the market at any
 * given moment of time.
 *
 * Trade event represents last trade information for <b>regular trading hours</b> (RTH) with an official volume and
 * turnover <b>for the whole trading day</b> identified by @ref TradeBase::getDayId() "dayId" field.
 *
 * TradeETH event is defined only for symbols (typically stocks and ETFs) with designated
 * <b>extended trading hours</b> (ETH, pre-market and post market trading sessions). It represents the last trade price
 * during ETH and also accumulated volume and turnover during ETH for current trading day.
 */
class DXFCPP_EXPORT TradeBase : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

    protected:
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

    struct TradeBaseData {
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

    TradeBaseData tradeBaseData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    public:
    /**
     * Maximum allowed sequence value.
     *
     * @see TradeBase::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    ///
    void assign(std::shared_ptr<EventType> event) override;

    /// Creates new trade event with default values.
    TradeBase() noexcept;

    /**
     * Creates a new trade event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TradeBase(const StringLike& eventSymbol) noexcept;

    /**
     * Returns time and sequence of the last trade packaged into a single long value.
     *
     * @return time and sequence of last trade.
     */
    std::int64_t getTimeSequence() const noexcept;

    /**
     * Changes time and sequence of last trade.
     * <b>Do not use this method directly.</b>
     * Change @ref TradeBase::setTime() "time" and/or @ref TradeBase::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see TradeBase::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Returns time of the last trade.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last trade.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Changes time of the last trade.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of the last trade.
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Returns time of the last trade in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last trade in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept;

    /**
     * Changes time of the last trade.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the last trade in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) noexcept;

    /**
     * Changes microseconds and nanoseconds time part of the last trade.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the last trade.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept;

    /**
     * Returns microseconds and nanoseconds time part of the last trade.
     *
     * @return microseconds and nanoseconds time part of the last trade.
     */
    std::int32_t getTimeNanoPart() const noexcept;

    /**
     * Returns sequence number of the last trade to distinguish trades that have the same
     * @ref TradeBase::getTime() "time". This sequence number does not have to be unique and
     * does not need to be sequential. Sequence can range from 0 to TradeBase::MAX_SEQUENCE.
     *
     * @return sequence of the last trade.
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes @ref TradeBase::getSequence() "sequence number" of the last trade.
     *
     * @param sequence the sequence.
     * @see TradeBase::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence);

    /**
     * Returns exchange code of the last trade.
     *
     * @return exchange code of the last trade.
     */
    std::int16_t getExchangeCode() const noexcept;

    /**
     * Returns exchange code of last trade as UTF8 string.
     *
     * @return exchange code of last trade as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept;

    /**
     * Changes exchange code of the last trade.
     *
     * @param exchangeCode exchange code of the last trade.
     */
    void setExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of the last trade.
     *
     * @param exchangeCode exchange code of the last trade.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Returns price of the last trade.
     *
     * @return price of the last trade.
     */
    double getPrice() const noexcept;

    /**
     * Changes price of the last trade.
     *
     * @param price price of the last trade.
     */
    void setPrice(double price) noexcept;

    /**
     * Returns size of the last trade as floating number with fractions.
     *
     * @return size of the last trade as floating number with fractions.
     */
    double getSize() const noexcept;

    /**
     * Changes size of the last trade as floating number with fractions.
     *
     * @param size size of the last trade as floating number with fractions.
     */
    void setSize(double size) noexcept;

    /**
     * Returns identifier of the current trading day.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return identifier of the current trading day.
     */
    std::int32_t getDayId() const noexcept;

    /**
     * Changes identifier of the current trading day.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param dayId identifier of the current trading day.
     */
    void setDayId(std::int32_t dayId) noexcept;

    /**
     * Returns total volume traded for a day as floating number with fractions.
     *
     * @return total volume traded for a day as floating number with fractions.
     */
    double getDayVolume() const noexcept;

    /**
     * Changes total volume traded for a day as floating number with fractions.
     *
     * @param dayVolume total volume traded for a day as floating number with fractions.
     */
    void setDayVolume(double dayVolume) noexcept;

    /**
     * Returns total turnover traded for a day.
     * Day VWAP can be computed with `getDayTurnover() / getDayVolume()`.
     *
     * @return total turnover traded for a day.
     */
    double getDayTurnover() const noexcept;

    /**
     * Changes total turnover traded for a day.
     *
     * @param dayTurnover total turnover traded for a day.
     */
    void setDayTurnover(double dayTurnover) noexcept;

    /**
     * Returns tick direction of the last trade.
     *
     * @return tick direction of the last trade.
     */
    const Direction &getTickDirection() const & noexcept;

    /**
     * Changes tick direction of the last trade.
     *
     * @param direction tick direction of the last trade.
     */
    void setTickDirection(const Direction &direction) noexcept;

    /**
     * Returns whether last trade was in extended trading hours.
     *
     * @return `true` if last trade was in extended trading hours.
     */
    bool isExtendedTradingHours() const noexcept;

    /**
     * Changes whether last trade was in extended trading hours.
     *
     * @param extendedTradingHours `true` if last trade was in extended trading hours.
     */
    void setExtendedTradingHours(bool extendedTradingHours) noexcept;

    /**
     * Returns change of the last trade.
     *
     * @return change of the last trade.
     */
    double getChange() const noexcept;

    /**
     * Changes change of the last trade.
     *
     * @param change price of the last trade.
     */
    void setChange(double change) noexcept;

    /**
     * Returns string representation of this trade event's fields.
     *
     * @return string representation of this trade event's fields.
     */
    std::string baseFieldsToString() const;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()