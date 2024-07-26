// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "ShortSaleRestriction.hpp"
#include "TradingStatus.hpp"

DXFCPP_BEGIN_NAMESPACE

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
        std::optional<std::string> description{};
        std::optional<std::string> statusReason{};
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

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the Profile object
    using Ptr = std::shared_ptr<Profile>;

    /// The alias to a type of unique pointer to the Profile object
    using Unique = std::unique_ptr<Profile>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

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

    /// Creates new profile event with default values.
    Profile() noexcept = default;

    /**
     * Creates new profile event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Profile(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

    /**
     * Returns description of the security instrument.
     *
     * @return description of the security instrument or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getDescription() const & noexcept {
        if (!data_.description) {
            return dxfcpp::String::NUL;
        }

        return data_.description.value();
    }

    /**
     * Returns description of the security instrument.
     *
     * @return description of the security instrument or std::nullopt
     */
    const std::optional<std::string> &getDescriptionOpt() const & noexcept {
        return data_.description;
    }

    /**
     * Changes description of the security instrument.
     *
     * @param description description of the security instrument.
     */
    void setDescription(std::string description) noexcept {
        data_.description = std::move(description);
    }

    /**
     * Returns short sale restriction of the security instrument.
     *
     * @return short sale restriction of the security instrument.
     */
    const ShortSaleRestriction &getShortSaleRestriction() const & noexcept {
        return ShortSaleRestriction::valueOf(getBits(data_.flags, SSR_MASK, SSR_SHIFT));
    }

    /**
     * Changes short sale restriction of the security instrument.
     *
     * @param restriction short sale restriction of the security instrument.
     */
    void setShortSaleRestriction(const ShortSaleRestriction &restriction) noexcept {
        data_.flags = setBits(data_.flags, SSR_MASK, SSR_SHIFT, restriction.getCode());
    }

    /**
     * Returns short sale restriction status of the security instrument.
     *
     * @return `true` if short sale of the security instrument is restricted.
     */
    bool isShortSaleRestricted() const noexcept {
        return getShortSaleRestriction() == ShortSaleRestriction::ACTIVE;
    }

    /**
     * Returns trading status of the security instrument.
     *
     * @return trading status of the security instrument.
     */
    const TradingStatus &getTradingStatus() const & noexcept {
        return TradingStatus::valueOf(getBits(data_.flags, STATUS_MASK, STATUS_SHIFT));
    }

    /**
     * Changes trading status of the security instrument.
     *
     * @param status trading status of the security instrument.
     */
    void setTradingStatus(const TradingStatus &status) noexcept {
        data_.flags = setBits(data_.flags, STATUS_MASK, STATUS_SHIFT, status.getCode());
    }

    /**
     * Returns trading halt status of the security instrument.
     *
     * @return `true` if trading of the security instrument is halted.
     */
    bool isTradingHalted() const noexcept {
        return getTradingStatus() == TradingStatus::HALTED;
    }

    /**
     * Returns description of the reason that trading was halted.
     *
     * @return description of the reason that trading was halted or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getStatusReason() const & noexcept {
        if (!data_.statusReason) {
            return dxfcpp::String::NUL;
        }

        return data_.statusReason.value();
    }

    /**
     * Returns description of the reason that trading was halted.
     *
     * @return description of the reason that trading was halted or std::nullopt.
     */
    const std::optional<std::string> &getStatusReasonOpt() const & noexcept {
        return data_.statusReason;
    }

    /**
     * Changes description of the reason that trading was halted.
     *
     * @param statusReason description of the reason that trading was halted.
     */
    void setStatusReason(std::string statusReason) noexcept {
        data_.statusReason = std::move(statusReason);
    }

    /**
     * Returns starting time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return starting time of the trading halt interval.
     */
    std::int64_t getHaltStartTime() const noexcept {
        return data_.haltStartTime;
    }

    /**
     * Changes starting time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param haltStartTime starting time of the trading halt interval.
     */
    void setHaltStartTime(std::int64_t haltStartTime) noexcept {
        data_.haltStartTime = haltStartTime;
    }

    /**
     * Returns ending time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return ending time of the trading halt interval.
     */
    std::int64_t getHaltEndTime() const noexcept {
        return data_.haltEndTime;
    }

    /**
     * Changes ending time of the trading halt interval.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param haltEndTime ending time of the trading halt interval.
     */
    void setHaltEndTime(std::int64_t haltEndTime) noexcept {
        data_.haltEndTime = haltEndTime;
    }

    /**
     * Returns the maximal (high) allowed price.
     *
     * @return the maximal (high) allowed price.
     */
    double getHighLimitPrice() const noexcept {
        return data_.highLimitPrice;
    }

    /**
     * Changes the maximal (high) allowed price.
     *
     * @param highLimitPrice the maximal (high) allowed price.
     */
    void setHighLimitPrice(double highLimitPrice) noexcept {
        data_.highLimitPrice = highLimitPrice;
    }

    /**
     * Returns the minimal (low) allowed price.
     *
     * @return the minimal (low) allowed price.
     */
    double getLowLimitPrice() const noexcept {
        return data_.lowLimitPrice;
    }

    /**
     * Changes the minimal (low) allowed price.
     *
     * @param lowLimitPrice the minimal (low) allowed price.
     */
    void setLowLimitPrice(double lowLimitPrice) noexcept {
        data_.lowLimitPrice = lowLimitPrice;
    }

    /**
     * Returns the maximal (high) price in last 52 weeks.
     *
     * @return the maximal (high) price in last 52 weeks.
     */
    double getHigh52WeekPrice() const noexcept {
        return data_.high52WeekPrice;
    }

    /**
     * Changes the maximal (high) price in last 52 weeks.
     *
     * @param high52WeekPrice the maximal (high) price in last 52 weeks.
     */
    void setHigh52WeekPrice(double high52WeekPrice) noexcept {
        data_.high52WeekPrice = high52WeekPrice;
    }

    /**
     * Returns the minimal (low) price in last 52 weeks.
     *
     * @return the minimal (low) price in last 52 weeks.
     */
    double getLow52WeekPrice() const noexcept {
        return data_.low52WeekPrice;
    }

    /**
     * Changes the minimal (low) price in last 52 weeks.
     *
     * @param low52WeekPrice the minimal (low) price in last 52 weeks.
     */
    void setLow52WeekPrice(double low52WeekPrice) noexcept {
        data_.low52WeekPrice = low52WeekPrice;
    }

    /**
     * Returns the correlation coefficient of the instrument to the S&P500 index.
     *
     * @return the correlation coefficient of the instrument to the S&P500 index.
     */
    double getBeta() const noexcept {
        return data_.beta;
    }

    /**
     * Changes the correlation coefficient of the instrument to the S&P500 index.
     *
     * @param beta the correlation coefficient of the instrument to the S&P500 index
     */
    void setBeta(double beta) noexcept {
        data_.beta = beta;
    }

    /**
     * Returns earnings per share (the company’s profits divided by the number of shares).
     *
     * @return earnings per share
     */
    double getEarningsPerShare() const noexcept {
        return data_.earningsPerShare;
    }

    /**
     * Changes Earnings per share (the company’s profits divided by the number of shares).
     *
     * @param earningsPerShare earnings per share
     */
    void setEarningsPerShare(double earningsPerShare) noexcept {
        data_.earningsPerShare = earningsPerShare;
    }

    /**
     * Returns frequency of cash dividends payments per year (calculated).
     *
     * @return Frequency of cash dividends payments per year
     */
    double getDividendFrequency() const noexcept {
        return data_.dividendFrequency;
    }

    /**
     * Changes frequency of cash dividends payments per year.
     *
     * @param dividendFrequency frequency of cash dividends payments per year
     */
    void setDividendFrequency(double dividendFrequency) noexcept {
        data_.dividendFrequency = dividendFrequency;
    }

    /**
     * Returns the amount of the last paid dividend.
     *
     * @return the amount of the last paid dividend
     */
    double getExDividendAmount() const noexcept {
        return data_.exDividendAmount;
    }

    /**
     * Changes the amount of the last paid dividend.
     *
     * @param exDividendAmount the amount of the last paid dividend
     */
    void setExDividendAmount(double exDividendAmount) noexcept {
        data_.exDividendAmount = exDividendAmount;
    }

    /**
     * Returns identifier of the day of the last dividend payment (ex-dividend date).
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return the identifier of the day of the last dividend payment
     */
    std::int32_t getExDividendDayId() const noexcept {
        return data_.exDividendDayId;
    }

    /**
     * Changes identifier of the day of the last dividend payment (ex-dividend date).
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param exDividendDayId identifier of the day of the last dividend payment
     */
    void setExDividendDayId(std::int32_t exDividendDayId) noexcept {
        data_.exDividendDayId = exDividendDayId;
    }

    /**
     * Returns the number of shares outstanding.
     *
     * @return shares outstanding
     */
    double getShares() const noexcept {
        return data_.shares;
    }

    /**
     * Changes the number of shares outstanding.
     *
     * @param shares shares outstanding.
     */
    void setShares(double shares) noexcept {
        data_.shares = shares;
    }

    /**
     * Returns free-float - the number of shares outstanding that are available to the public for trade.
     *
     * @return free-float
     */
    double getFreeFloat() const noexcept {
        return data_.freeFloat;
    }

    /**
     * Changes free-float - the number of shares outstanding that are available to the public for trade
     * @param freeFloat the number of shares outstanding that are available to the public for trade
     */
    void setFreeFloat(double freeFloat) noexcept {
        data_.freeFloat = freeFloat;
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