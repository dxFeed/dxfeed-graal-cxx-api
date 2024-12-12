// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "PriceType.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Summary information snapshot about the trading session including session highs, lows, etc.
 * It represents the most recent information that is available about the trading session in
 * the market at any given moment of time.
 */
class DXFCPP_EXPORT Summary final : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..4     3    2    1    0
     * +--------+----+----+----+----+
     * |        |  Close  |PrevClose|
     * +--------+----+----+----+----+
     */

    // PRICE_TYPE values are taken from PriceType enum.
    static const std::uint32_t DAY_CLOSE_PRICE_TYPE_MASK = 3;
    static const std::uint32_t DAY_CLOSE_PRICE_TYPE_SHIFT = 2;

    // PRICE_TYPE values are taken from PriceType enum.
    static const std::uint32_t PREV_DAY_CLOSE_PRICE_TYPE_MASK = 3;
    static const std::uint32_t PREV_DAY_CLOSE_PRICE_TYPE_SHIFT = 0;

    struct Data {
        std::int32_t dayId{};
        double dayOpenPrice = math::NaN;
        double dayHighPrice = math::NaN;
        double dayLowPrice = math::NaN;
        double dayClosePrice = math::NaN;
        std::int32_t prevDayId{};
        double prevDayClosePrice = math::NaN;
        double prevDayVolume = math::NaN;
        std::int64_t openInterest{};
        std::int32_t flags{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the Summary object
    using Ptr = std::shared_ptr<Summary>;

    /// The alias to a type of unique pointer to the Summary object
    using Unique = std::unique_ptr<Summary>;

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

    /// Creates new summary event with default values.
    Summary() noexcept = default;

    /**
     * Creates new summary event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Summary(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)) {
    }

    /**
     * Returns identifier of the day that this summary represents.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return identifier of the day that this summary represents.
     */
    std::int32_t getDayId() const noexcept {
        return data_.dayId;
    }

    /**
     * Changes identifier of the day that this summary represents.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param dayId identifier of the day that this summary represents.
     */
    void setDayId(std::int32_t dayId) noexcept {
        data_.dayId = dayId;
    }

    /**
     * Returns the first (open) price for the day.
     *
     * @return the first (open) price for the day.
     */
    double getDayOpenPrice() const noexcept {
        return data_.dayOpenPrice;
    }

    /**
     * Changes the first (open) price for the day.
     *
     * @param dayOpenPrice the first (open) price for the day.
     */
    void setDayOpenPrice(double dayOpenPrice) noexcept {
        data_.dayOpenPrice = dayOpenPrice;
    }

    /**
     * Returns the maximal (high) price for the day.
     *
     * @return the maximal (high) price for the day.
     */
    double getDayHighPrice() const noexcept {
        return data_.dayHighPrice;
    }

    /**
     * Changes the maximal (high) price for the day.
     *
     * @param dayHighPrice the maximal (high) price for the day.
     */
    void setDayHighPrice(double dayHighPrice) noexcept {
        data_.dayHighPrice = dayHighPrice;
    }

    /**
     * Returns the minimal (low) price for the day.
     *
     * @return the minimal (low) price for the day.
     */
    double getDayLowPrice() const noexcept {
        return data_.dayLowPrice;
    }

    /**
     * Changes the minimal (low) price for the day.
     *
     * @param dayLowPrice the minimal (low) price for the day.
     */
    void setDayLowPrice(double dayLowPrice) noexcept {
        data_.dayLowPrice = dayLowPrice;
    }

    /**
     * Returns the last (close) price for the day.
     *
     * @return the last (close) price for the day.
     */
    double getDayClosePrice() const noexcept {
        return data_.dayClosePrice;
    }

    /**
     * Changes the last (close) price for the day.
     *
     * @param dayClosePrice the last (close) price for the day.
     */
    void setDayClosePrice(double dayClosePrice) noexcept {
        data_.dayClosePrice = dayClosePrice;
    }

    /**
     * Returns the price type of the last (close) price for the day.
     *
     * @return the price type of the last (close) price for the day.
     */
    const PriceType &getDayClosePriceType() const & noexcept {
        return PriceType::valueOf(getBits(data_.flags, DAY_CLOSE_PRICE_TYPE_MASK, DAY_CLOSE_PRICE_TYPE_SHIFT));
    }

    /**
     * Changes the price type of the last (close) price for the day.
     *
     * @param type the price type of the last (close) price for the day.
     */
    void setDayClosePriceType(const PriceType &type) noexcept {
        data_.flags = setBits(data_.flags, DAY_CLOSE_PRICE_TYPE_MASK, DAY_CLOSE_PRICE_TYPE_SHIFT, type.getCode());
    }

    /**
     * Returns identifier of the previous day that this summary represents.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @return identifier of the previous day that this summary represents.
     */
    std::int32_t getPrevDayId() const noexcept {
        return data_.prevDayId;
    }

    /**
     * Changes identifier of the previous day that this summary represents.
     * Identifier of the day is the number of days passed since January 1, 1970.
     *
     * @param prevDayId identifier of the previous day that this summary represents.
     */
    void setPrevDayId(std::int32_t prevDayId) noexcept {
        data_.prevDayId = prevDayId;
    }

    /**
     * Returns the last (close) price for the previous day.
     *
     * @return the last (close) price for the previous day.
     */
    double getPrevDayClosePrice() const noexcept {
        return data_.prevDayClosePrice;
    }

    /**
     * Changes the last (close) price for the previous day.
     *
     * @param prevDayClosePrice the last (close) price for the previous day.
     */
    void setPrevDayClosePrice(double prevDayClosePrice) noexcept {
        data_.prevDayClosePrice = prevDayClosePrice;
    }

    /**
     * Returns the price type of the last (close) price for the previous day.
     *
     * @return the price type of the last (close) price for the previous day.
     */
    const PriceType &getPrevDayClosePriceType() const & noexcept {
        return PriceType::valueOf(
            getBits(data_.flags, PREV_DAY_CLOSE_PRICE_TYPE_MASK, PREV_DAY_CLOSE_PRICE_TYPE_SHIFT));
    }

    /**
     * Changes the price type of the last (close) price for the previous day.
     *
     * @param type the price type of the last (close) price for the previous day.
     */
    void setPrevDayClosePriceType(const PriceType &type) noexcept {
        data_.flags =
            setBits(data_.flags, PREV_DAY_CLOSE_PRICE_TYPE_MASK, PREV_DAY_CLOSE_PRICE_TYPE_SHIFT, type.getCode());
    }

    /**
     * Returns total volume traded for the previous day.
     *
     * @return total volume traded for the previous day.
     */
    double getPrevDayVolume() const noexcept {
        return data_.prevDayVolume;
    }

    /**
     * Changes total volume traded for the previous day.
     *
     * @param prevDayVolume total volume traded for the previous day.
     */
    void setPrevDayVolume(double prevDayVolume) noexcept {
        data_.prevDayVolume = prevDayVolume;
    }

    /**
     * Returns open interest of the symbol as the number of open contracts.
     *
     * @return open interest of the symbol as the number of open contracts.
     */
    std::int64_t getOpenInterest() const noexcept {
        return data_.openInterest;
    }

    /**
     * Changes open interest of the symbol as the number of open contracts.
     *
     * @param openInterest open interest of the symbol as the number of open contracts.
     */
    void setOpenInterest(std::int64_t openInterest) noexcept {
        data_.openInterest = openInterest;
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