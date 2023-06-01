// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "ShortSaleRestriction.hpp"
#include "TradingStatus.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Profile information snapshot that contains security instrument description.
 * It represents the most recent information that is available about the traded security
 * on the market at any given moment of time.
 */
class DXFCPP_EXPORT Profile final : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..4     3    2    1    0
     * +--------+----+----+----+----+
     * |        |   SSR   |  Status |
     * +--------+----+----+----+----+
     */

    // SSR values are taken from ShortSaleRestriction enum.
    static const std::uint32_t SSR_MASK = 3;
    static const std::uint32_t SSR_SHIFT = 2;

    // STATUS values are taken from TradingStatus enum.
    static const std::uint32_t STATUS_MASK = 3;
    static const std::uint32_t STATUS_SHIFT = 0;

    struct Data {
        std::string description{};
        std::string statusReason{};
        std::int64_t haltStartTime{};
        std::int64_t haltEndTime{};
        double highLimitPrice = math::NaN;
        double lowLimitPrice = math::NaN;
        double high52WeekPrice = math::NaN;
        double low52WeekPrice = math::NaN;
        double beta = math::NaN;
        double earningsPerShare = math::NaN;
        double dividendFrequency = math::NaN;
        double exDividendAmount = math::NaN;
        std::int32_t exDividendDayId{};
        double shares = math::NaN;
        double freeFloat = math::NaN;
        std::int32_t flags{};
    };

    Data data_{};

    static std::shared_ptr<Profile> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new profile event with default values.
    Profile() noexcept = default;

    /**
     * Creates new profile event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Profile(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {}

    /**
     * Returns description of the security instrument.
     *
     * @return description of the security instrument.
     */
    const std::string &getDescription() const & { return data_.description; }

    /**
     * Changes description of the security instrument.
     *
     * @param description description of the security instrument.
     */
    void setDescription(std::string description) { data_.description = std::move(description); }

    /**
     * Returns short sale restriction of the security instrument.
     *
     * @return short sale restriction of the security instrument.
     */
    const ShortSaleRestriction &getShortSaleRestriction() const & {
        return ShortSaleRestriction::valueOf(getBits(data_.flags, SSR_MASK, SSR_SHIFT));
    }

    /**
     * Changes short sale restriction of the security instrument.
     *
     * @param restriction short sale restriction of the security instrument.
     */
    void setShortSaleRestriction(const ShortSaleRestriction &restriction) {
        data_.flags = setBits(data_.flags, SSR_MASK, SSR_SHIFT, restriction.getCode());
    }

    /**
     * Returns short sale restriction status of the security instrument.
     *
     * @return `true` if short sale of the security instrument is restricted.
     */
    bool isShortSaleRestricted() const { return getShortSaleRestriction() == ShortSaleRestriction::ACTIVE; }

    /**
     * Returns trading status of the security instrument.
     *
     * @return trading status of the security instrument.
     */
    const TradingStatus &getTradingStatus() const & {
        return TradingStatus::valueOf(getBits(data_.flags, STATUS_MASK, STATUS_SHIFT));
    }

    /**
     * Changes trading status of the security instrument.
     *
     * @param status trading status of the security instrument.
     */
    void setTradingStatus(const TradingStatus &status) {
        data_.flags = setBits(data_.flags, STATUS_MASK, STATUS_SHIFT, status.getCode());
    }

    /**
     * Returns trading halt status of the security instrument.
     *
     * @return `true` if trading of the security instrument is halted.
     */
    bool isTradingHalted() const { return getTradingStatus() == TradingStatus::HALTED; }

    /**
     * Returns description of the reason that trading was halted.
     *
     * @return description of the reason that trading was halted.
     */
    const std::string &getStatusReason() const & { return data_.statusReason; }

    /**
     * Changes description of the reason that trading was halted.
     *
     * @param statusReason description of the reason that trading was halted.
     */
    void setStatusReason(std::string statusReason) { data_.statusReason = std::move(statusReason); }

    /**
     * Returns starting time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return starting time of the trading halt interval.
     */
    std::int64_t getHaltStartTime() const { return data_.haltStartTime; }

    /**
     * Changes starting time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param haltStartTime starting time of the trading halt interval.
     */
    void setHaltStartTime(std::int64_t haltStartTime) { data_.haltStartTime = haltStartTime; }

    /**
     * Returns ending time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return ending time of the trading halt interval.
     */
    std::int64_t getHaltEndTime() const { return data_.haltEndTime; }

    /**
     * Changes ending time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param haltEndTime ending time of the trading halt interval.
     */
    void setHaltEndTime(std::int64_t haltEndTime) { data_.haltEndTime = haltEndTime; }

    /**
     * Returns the maximal (high) allowed price.
     *
     * @return the maximal (high) allowed price.
     */
    double getHighLimitPrice() const { return data_.highLimitPrice; }

    /**
     * Changes the maximal (high) allowed price.
     *
     * @param highLimitPrice the maximal (high) allowed price.
     */
    void setHighLimitPrice(double highLimitPrice) { data_.highLimitPrice = highLimitPrice; }

    /**
     * Returns the minimal (low) allowed price.
     *
     * @return the minimal (low) allowed price.
     */
    double getLowLimitPrice() const { return data_.lowLimitPrice; }

    /**
     * Changes the minimal (low) allowed price.
     *
     * @param lowLimitPrice the minimal (low) allowed price.
     */
    void setLowLimitPrice(double lowLimitPrice) { data_.lowLimitPrice = lowLimitPrice; }

    /**
     * Returns the maximal (high) price in last 52 weeks.
     *
     * @return the maximal (high) price in last 52 weeks.
     */
    double getHigh52WeekPrice() const { return data_.high52WeekPrice; }

    /**
     * Changes the maximal (high) price in last 52 weeks.
     *
     * @param high52WeekPrice the maximal (high) price in last 52 weeks.
     */
    void setHigh52WeekPrice(double high52WeekPrice) { data_.high52WeekPrice = high52WeekPrice; }

    /**
     * Returns the minimal (low) price in last 52 weeks.
     *
     * @return the minimal (low) price in last 52 weeks.
     */
    double getLow52WeekPrice() const { return data_.low52WeekPrice; }

    /**
     * Changes the minimal (low) price in last 52 weeks.
     *
     * @param low52WeekPrice the minimal (low) price in last 52 weeks.
     */
    void setLow52WeekPrice(double low52WeekPrice) { data_.low52WeekPrice = low52WeekPrice; }

    /**
     * Returns the correlation coefficient of the instrument to the S&P500 index.
     *
     * @return the correlation coefficient of the instrument to the S&P500 index.
     */
    double getBeta() const { return data_.beta; }

    /**
     * Changes the correlation coefficient of the instrument to the S&P500 index.
     *
     * @param beta the correlation coefficient of the instrument to the S&P500 index
     */
    void setBeta(double beta) { data_.beta = beta; }

    /**
     * Returns earnings per share (the company’s profits divided by the number of shares).
     *
     * @return earnings per share
     */
    double getEarningsPerShare() const { return data_.earningsPerShare; }

    /**
     * Changes Earnings per share (the company’s profits divided by the number of shares).
     *
     * @param earningsPerShare earnings per share
     */
    void setEarningsPerShare(double earningsPerShare) { data_.earningsPerShare = earningsPerShare; }

    /**
     * Returns frequency of cash dividends payments per year (calculated).
     *
     * @return Frequency of cash dividends payments per year
     */
    double getDividendFrequency() const { return data_.dividendFrequency; }

    /**
     * Changes frequency of cash dividends payments per year.
     *
     * @param dividendFrequency frequency of cash dividends payments per year
     */
    void setDividendFrequency(double dividendFrequency) { data_.dividendFrequency = dividendFrequency; }

    /**
     * Returns the amount of the last paid dividend.
     *
     * @return the amount of the last paid dividend
     */
    double getExDividendAmount() const { return data_.exDividendAmount; }

    /**
     * Changes the amount of the last paid dividend.
     *
     * @param exDividendAmount the amount of the last paid dividend
     */
    void setExDividendAmount(double exDividendAmount) { data_.exDividendAmount = exDividendAmount; }

    /**
     * Returns identifier of the day of the last dividend payment (ex-dividend date).
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return the identifier of the day of the last dividend payment
     */
    std::int32_t getExDividendDayId() const { return data_.exDividendDayId; }

    /**
     * Changes identifier of the day of the last dividend payment (ex-dividend date).
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param exDividendDayId identifier of the day of the last dividend payment
     */
    void setExDividendDayId(std::int32_t exDividendDayId) { data_.exDividendDayId = exDividendDayId; }

    /**
     * Returns the number of shares outstanding.
     *
     * @return shares outstanding
     */
    double getShares() const { return data_.shares; }

    /**
     * Changes the number of shares outstanding.
     *
     * @param shares shares outstanding.
     */
    void setShares(double shares) { data_.shares = shares; }

    /**
     * Returns free-float - the number of shares outstanding that are available to the public for trade.
     *
     * @return free-float
     */
    double getFreeFloat() const { return data_.freeFloat; }

    /**
     * Changes free-float - the number of shares outstanding that are available to the public for trade
     * @param freeFloat the number of shares outstanding that are available to the public for trade
     */
    void setFreeFloat(double freeFloat) { data_.freeFloat = freeFloat; }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp