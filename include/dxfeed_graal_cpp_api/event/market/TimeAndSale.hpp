// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"
#include "../TimeSeriesEvent.hpp"
#include "./MarketEvent.hpp"
#include "./Side.hpp"
#include "./TimeAndSaleType.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;
class OptionSale;

/**
 * Time and Sale represents a trade or other market event with price, like market open/close price, etc.
 * Time and Sales are intended to provide information about trades <b>in a continuous time slice</b>
 * (unlike Trade events which are supposed to provide a snapshot about the <b>current last</b> trade).
 *
 * <p> Time and Sale events have unique @ref TimeAndSale::getIndex() "index" which can be used for later
 * correction/cancellation processing.
 *
 * Some time and sale sources provide a consistent view of the set of known time and sales
 * for a given time range when used with DXFeedTimeSeriesSubscription}.
 * The corresponding information is carried in @ref TimeAndSale::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for time and sales, thus
 * @ref TimeAndSale::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p> Regular subscription via DXFeedSubscription produces a stream of time and
 * sale events as they happen, and their @ref TimeAndSale::getEventFlags() "eventFlags" are always zero.
 *
 * Publishing of time and sales events follows the general rules explained in TimeSeriesEvent class
 * documentation.
 *
 * This event is implemented on top of QDS record `TimeAndSale`.
 */
class DXFCPP_EXPORT TimeAndSale final : public MarketEvent, public TimeSeriesEvent {
    friend struct EventMapper;
    friend class OptionSale;

    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..16   15...8    7    6    5    4    3    2    1    0
     * +--------+--------+----+----+----+----+----+----+----+----+
     * |        |   TTE  |    |  Side   | SL | ETH| VT |  Type   |
     * +--------+--------+----+----+----+----+----+----+----+----+
     */

    // TTE (TradeThroughExempt) values are ASCII chars in [0, 255].
    static constexpr std::uint32_t TTE_MASK = 0xFFU;
    static constexpr std::uint32_t TTE_SHIFT = 8U;

    // SIDE values are taken from the Side enum.
    static constexpr std::uint32_t SIDE_MASK = 3U;
    static constexpr std::uint32_t SIDE_SHIFT = 5U;

    static constexpr std::uint32_t SPREAD_LEG = 1U << 4U;
    static constexpr std::uint32_t ETH = 1U << 3U;
    static constexpr std::uint32_t VALID_TICK = 1U << 2U;

    // TYPE values are taken from the TimeAndSaleType enum.
    static constexpr std::uint32_t TYPE_MASK = 3U;
    static constexpr std::uint32_t TYPE_SHIFT = 0;

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
        std::int32_t timeNanoPart{};
        std::int16_t exchangeCode{};
        double price = math::NaN;
        double size = math::NaN;
        double bidPrice = math::NaN;
        double askPrice = math::NaN;
        std::optional<std::string> exchangeSaleConditions{};
        std::int32_t flags{};
        std::optional<std::string> buyer{};
        std::optional<std::string> seller{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    /// The alias to a type of shared pointer to the TimeAndSale object
    using Ptr = std::shared_ptr<TimeAndSale>;

    /// The alias to a type of unique pointer to the TimeAndSale object
    using Unique = std::unique_ptr<TimeAndSale>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    /**
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
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

    /// Creates new time and sale event with default values.
    TimeAndSale() noexcept;

    /**
     * Creates a new time and sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TimeAndSale(const StringLike &eventSymbol) noexcept;

    ///
    const IndexedEventSource &getSource() const & noexcept override;

    ///
    std::int32_t getEventFlags() const noexcept override;

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override;

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override;

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override;

    /**
     * Returns a unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence".
     * Changing either time or sequence changes the event index.
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override;

    /**
     * Changes the unique per-symbol index of this event.
     * The index is composed of @ref ::getTime() "time" and @ref ::getSequence() "sequence" and
     * invocation of this method changes time and sequence.
     * <b>Do not use this method directly.</b>
     * Change @ref ::setTime() "time" and/or @ref ::setSequence() "sequence".
     *
     * @param index the event index.
     * @see ::getIndex()
     */
    void setIndex(std::int64_t index) override;

    /**
     * Returns the timestamp of the event in milliseconds.
     *
     * @return timestamp of the event in milliseconds
     */
    std::int64_t getTime() const noexcept override;

    /**
     * Changes the timestamp of the event in milliseconds.
     *
     * @param time timestamp of the event in milliseconds.
     * @see ::getTime()
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Returns time of the original event in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the original event in nanoseconds.
     */
    std::int64_t getTimeNanos() const noexcept;

    /**
     * Changes time of the original event.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos time of the original event in nanoseconds.
     */
    void setTimeNanos(std::int64_t timeNanos) noexcept;

    /**
     * Changes microseconds and nanoseconds time part of the original event.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of the original event.
     */
    void setTimeNanoPart(std::int32_t timeNanoPart) noexcept;

    /**
     * Returns microseconds and nanoseconds time part of the original event.
     *
     * @return microseconds and nanoseconds time part of the original event.
     */
    std::int32_t getTimeNanoPart() const noexcept;

    /**
     * Returns the sequence number of this event to distinguish events that have the same @ref ::getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential.
     * Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return The sequence number of this event
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes @ref ::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @see ::getSequence()
     * @throws InvalidArgumentException
     */
    void setSequence(std::int32_t sequence);

    /**
     * Returns exchange code of this time and sale event.
     *
     * @return exchange code of this time and sale event.
     */
    std::int16_t getExchangeCode() const noexcept;

    /**
     * Returns exchange code of this time and sale event as UTF8 string.
     *
     * @return exchange code of this time and sale event as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept;

    /**
     * Changes exchange code of this time and sale event.
     *
     * @param exchangeCode exchange code of this time and sale event.
     */
    void setExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of this time and sale event.
     *
     * @param exchangeCode exchange code of this time and sale event.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Returns price of this time and sale event.
     *
     * @return price of this time and sale event.
     */
    double getPrice() const noexcept;

    /**
     * Changes price of this time and sale event.
     *
     * @param price price of this time and sale event.
     */
    void setPrice(double price) noexcept;

    /**
     * Returns size of this time and sale event.
     *
     * @return size of this time and sale event.
     */
    double getSize() const noexcept;

    /**
     * Changes size of this time and sale event.
     *
     * @param size size of this time and sale event.
     */
    void setSize(double size) noexcept;

    /**
     * Returns the current bid price on the market when this time and sale event had occurred.
     *
     * @return the current bid price on the market when this time and sale event had occurred.
     */
    double getBidPrice() const noexcept;

    /**
     * Changes the current bid price on the market when this time and sale event had occurred.
     *
     * @param bidPrice the current bid price on the market when this time and sale event had occurred.
     */
    void setBidPrice(double bidPrice) noexcept;

    /**
     * Returns the current ask price on the market when this time and sale event had occurred.
     *
     * @return the current ask price on the market when this time and sale event had occurred.
     */
    double getAskPrice() const noexcept;

    /**
     * Changes price of this time and sale event.the current ask price on the market when this time and sale event had
     * occurred.
     *
     * @param askPrice the current ask price on the market when this time and sale event had occurred.
     */
    void setAskPrice(double askPrice) noexcept;

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getExchangeSaleConditions() const & noexcept;

    /**
     * Returns sale conditions provided for this event by data feed.
     * This field format is specific for every particular data feed.
     *
     * @return sale conditions or std::nullopt
     */
    const std::optional<std::string> &getExchangeSaleConditionsOpt() const & noexcept;

    /**
     * Changes sale conditions provided for this event by data feed.
     *
     * @param exchangeSaleConditions sale conditions.
     */
    void setExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept;

    /**
     * Returns TradeThroughExempt flag of this time and sale event.
     *
     * @return TradeThroughExempt flag of this time and sale event.
     */
    char getTradeThroughExempt() const noexcept;

    /**
     * Changes TradeThroughExempt flag of this time and sale event.
     *
     * @param tradeThroughExempt TradeThroughExempt flag of this time and sale event.
     */
    void setTradeThroughExempt(char tradeThroughExempt);

    /**
     * Returns aggressor side of this time and sale event.
     *
     * @return aggressor side of this time and sale event.
     */
    const Side &getAggressorSide() const & noexcept;

    /**
     * Changes aggressor side of this time and sale event.
     *
     * @param side aggressor side of this time and sale event.
     */
    void setAggressorSide(const Side &side) noexcept;

    /**
     * Returns whether this event represents a spread leg.
     *
     * @return `true` if this event represents a spread leg.
     */
    bool isSpreadLeg() const noexcept;

    /**
     * Changes whether this event represents a spread leg.
     *
     * @param spreadLeg `true` if this event represents a spread leg.
     */
    void setSpreadLeg(bool spreadLeg) noexcept;

    /**
     * Returns whether this event represents an extended trading hours sale.
     *
     * @return `true` if this event represents an extended trading hours sale.
     */
    bool isExtendedTradingHours() const noexcept;

    /**
     * Changes whether this event represents an extended trading hours sale.
     *
     * @param extendedTradingHours `true` if this event represents an extended trading hours sale.
     */
    void setExtendedTradingHours(bool extendedTradingHours) noexcept;

    /**
     * Returns whether this event represents a valid intraday tick.
     * Note, that a correction for a previously distributed valid tick represents a new valid tick itself,
     * but a cancellation of a previous valid tick does not.
     *
     * @return `true` if this event represents a valid intraday tick.
     */
    bool isValidTick() const noexcept;

    /**
     * Changes whether this event represents a valid intraday tick.
     *
     * @param validTick `true` if this event represents a valid intraday tick.
     */
    void setValidTick(bool validTick) noexcept;

    /**
     * Returns type of this time and sale event.
     *
     * @return type of this time and sale event.
     */
    const TimeAndSaleType &getType() const & noexcept;

    /**
     * Changes type of this time and sale event.
     *
     * @param type type of this time and sale event.
     */
    void setType(const TimeAndSaleType &type) noexcept;

    /**
     * Returns whether this is a new event (not cancellation or correction).
     * It is `true` for newly created time and sale event.
     *
     * @return `true` if this is a new event (not cancellation or correction).
     */
    bool isNew() const noexcept;

    /**
     * Returns whether this is a correction of a previous event.
     * It is `false` for newly created time and sale event.
     *
     * @return `true` if this is a correction of a previous event
     */
    bool isCorrection() const noexcept;

    /**
     * Returns whether this is a cancellation of a previous event.
     * It is `false` for newly created time and sale event.
     * @return `true` if this is a cancellation of a previous event
     */
    bool isCancel() const noexcept;

    /**
     * Returns buyer of this time and sale event.
     *
     * @return buyer of this time and sale event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getBuyer() const & noexcept;

    /**
     * Returns buyer of this time and sale event.
     *
     * @return buyer of this time and sale event or std::nullopt
     */
    const std::optional<std::string> &getBuyerOpt() const & noexcept;

    /**
     * Changes buyer of this time and sale event.
     *
     * @param buyer buyer of this time and sale event.
     */
    void setBuyer(const StringLike &buyer) noexcept;

    /**
     * Returns seller of this time and sale event.
     *
     * @return seller of this time and sale event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getSeller() const & noexcept;

    /**
     * Returns seller of this time and sale event.
     *
     * @return seller of this time and sale event or std::nullopt
     */
    const std::optional<std::string> &getSellerOpt() const & noexcept;

    /**
     * Changes seller of this time and sale event.
     *
     * @param seller seller of this time and sale event.
     */
    void setSeller(const StringLike &seller) noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()