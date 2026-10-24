// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEvent.hpp"
#include "../IndexedEventSource.hpp"
#include "./MarketEvent.hpp"

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

/**
 * The Market Maker event captures a snapshot of aggregated top quotes from various market participants,
 * such as market makers, for a specific symbol. The collection of Market Maker events for a symbol
 * provides aggregate data for a given price level or the best bid and offer from selected market makers
 * at any given moment.
 *
 * <p>The Market Maker events offer varying levels of detail based on the data sources.
 * Initially created for Level 2 data, where each market maker's best quote is represented, the Market Maker event
 * aggregates these quotes to present a comprehensive view of the market.
 * In addition to Level 2 data, the Market Maker event can also represent other aggregated order books with
 * limited depth (e.g., top 5 price levels). In these instances, each price level is treated as a distinct
 * "market maker" with an assigned identifier (e.g., 1, 2, 3, 4, 5), enabling a simplified and consistent
 * representation across different data sources.
 *
 * <p>For more detailed order book information or full market depth, the Order event should be utilized.
 *
 * <h3>Properties</h3>
 *
 * MarketMaker event has the following properties:
 *
 * <ul>
 * <li>@ref MarketMaker::getEventSymbol() "eventSymbol" - underlying symbol of this event;
 * <li>@ref MarketMaker::getSource() "source" - source of this event;
 * <li>@ref MarketMaker::getEventFlags() "eventFlags" - transactional event flags;
 * <li>@ref MarketMaker::getIndex() "index" - unique per-symbol index of this event;
 * <li>@ref MarketMaker::getTime() "time" - time of the last bid or ask change;
 * <li>@ref MarketMaker::getExchangeCode() "exchangeCode" - exchange code of this event;
 * <li>@ref MarketMaker::getMarketMaker() "marketMaker" - market maker or other aggregate identifier of this event;
 * <li>@ref MarketMaker::getBidTime() "bidTime" - time of the last bid change;
 * <li>@ref MarketMaker::getBidPrice() "bidPrice" - bid price;
 * <li>@ref MarketMaker::getBidSize() "bidSize" - bid size;
 * <li>@ref MarketMaker::getBidCount() "bidCount" - number of individual bid orders in this market maker event;
 * <li>@ref MarketMaker::getAskTime() "askTime" - time of the last ask change;
 * <li>@ref MarketMaker::getAskPrice() "askPrice" - ask price;
 * <li>@ref MarketMaker::getAskSize() "askSize" - ask size;
 * <li>@ref MarketMaker::getAskCount() "askCount" - number of individual ask orders in this market maker event.
 * </ul>
 *
 * <h3>Event flags, transactions and snapshots</h3>
 *
 * Certain Market Maker event sources provide a consistent view of the aggregated order book based on identifiers
 * such as "market maker", price level number, or bank identifier. Updates may involve multiple changes
 * to price levels that should be processed simultaneously. The corresponding information is carried in
 * @ref MarketMaker::getEventFlags() "eventFlags" property. See IndexedEvent class documentation for details.
 *
 * <h3>Publishing Market Maker</h3>
 *
 * When publishing a Market Maker event, you can use MarketMaker::setIndex() to set the index directly, or use
 * MarketMaker::setExchangeCode() with MarketMaker::setMarketMaker(). The index is a bitwise representation of these
 * two fields.
 *
 * <p>A snapshot has to be published in the <em>descending</em> order of @ref MarketMaker::getIndex() "index",
 * starting with an event with the largest index and marking it with EventFlag::SNAPSHOT_BEGIN, and finishing the
 * snapshot with an event that has zero index. EventFlag::SNAPSHOT_END is optional during publishing.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `MarketMaker`.
 *
 * <h3>Bid & Ask time precision</h3>
 *
 * @ref MarketMaker::getBidTime() "bidTime" and @ref MarketMaker::getAskTime() "askTime" are transferred with
 * <em>seconds</em> precision by default. The `dxscheme.bat` system property configures it: `seconds` (default) or
 * `millis`.
 */
class DXFCPP_EXPORT MarketMaker final : public MarketEvent, public IndexedEvent {
    friend struct EventMapper;

    /*
     * Index field contains exchange code and market maker id (MMID) in the following way:
     *   63..48        47..32          31..0
     * +--------+---------------+--------------+
     * |        | Exchange code | MMID         |
     * +--------+---------------+--------------+
     */

    // EXCHANGE values are char.
    static constexpr std::uint64_t EXCHANGE_MASK = 0xFFFFULL;
    static constexpr std::uint64_t EXCHANGE_SHIFT = 32ULL;

    // MMID values are strings in the QDS ShortString format.
    static constexpr std::uint64_t MMID_MASK = 0xFFFFFFFFULL;
    static constexpr std::uint64_t MMID_SHIFT = 0ULL;

    struct Data {
        std::int32_t eventFlags{};
        std::int64_t index{};
        std::int64_t bidTime{};
        double bidPrice = math::NaN;
        double bidSize = math::NaN;
        std::int64_t bidCount{};
        std::int64_t askTime{};
        double askPrice = math::NaN;
        double askSize = math::NaN;
        std::int64_t askCount{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    /// The alias to a type of shared pointer to the MarketMaker object
    using Ptr = std::shared_ptr<MarketMaker>;

    /// The alias to a type of unique pointer to the MarketMaker object
    using Unique = std::unique_ptr<MarketMaker>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

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

    /// Creates new market maker event with default values.
    MarketMaker() noexcept;

    /**
     * Creates a new market maker event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit MarketMaker(const StringLike &eventSymbol) noexcept;

    // MarketEvent methods

    /**
     * Changes an event's symbol and returns the current market maker.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current market maker.
     */
    MarketMaker &withEventSymbol(const StringLike &eventSymbol) noexcept;

    /**
     * Changes event's creation time and returns the current market maker.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current market maker.
     */
    MarketMaker &withEventTime(std::int64_t eventTime) noexcept;

    // IndexedEvent methods

    /**
     * Returns the source of this event. It is always @ref IndexedEventSource::DEFAULT "DEFAULT".
     *
     * @return The source of this event.
     */
    const IndexedEventSource &getSource() const & noexcept override;

    ///
    std::int32_t getEventFlags() const noexcept override;

    ///
    EventFlagsMask getEventFlagsMask() const noexcept override;

    ///
    void setEventFlags(std::int32_t eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current market maker.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current market maker.
     */
    MarketMaker &withEventFlags(std::int32_t eventFlags) noexcept;

    ///
    void setEventFlags(const EventFlagsMask &eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current market maker.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current market maker.
     */
    MarketMaker &withEventFlags(const EventFlagsMask &eventFlags) noexcept;

    /**
     * Returns a unique per-symbol index of this event.
     * The index is composed of @ref MarketMaker::getExchangeCode() "exchange code" and
     * @ref MarketMaker::getMarketMaker() "market maker".
     *
     * @return unique index of this event.
     */
    std::int64_t getIndex() const noexcept override;

    /**
     * Changes the unique per-symbol index of this event.
     * The index is composed of @ref MarketMaker::getExchangeCode() "exchange code" and
     * @ref MarketMaker::getMarketMaker() "market maker", and invocation of this method changes both.
     *
     * @param index the event index.
     * @throws InvalidArgumentException if the index is negative.
     * @see MarketMaker::getIndex()
     */
    void setIndex(std::int64_t index) override;

    /**
     * Changes unique per-symbol index of this event.
     * Returns the current market maker.
     *
     * @param index unique per-symbol index of this market maker.
     * @return The current market maker.
     * @throws InvalidArgumentException if the index is negative.
     */
    MarketMaker &withIndex(std::int64_t index);

    // MarketMaker methods

    /**
     * Returns time of the last bid or ask change: the maximum of @ref MarketMaker::getBidTime() "bidTime" and
     * @ref MarketMaker::getAskTime() "askTime".
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid or ask change.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Returns exchange code of this market maker event.
     *
     * @return exchange code of this market maker event.
     */
    std::int16_t getExchangeCode() const noexcept;

    /**
     * Returns exchange code of this market maker event as UTF8 string.
     *
     * @return exchange code of this market maker event as UTF8 string.
     */
    std::string getExchangeCodeString() const noexcept;

    /**
     * Changes exchange code of this market maker event. It does not change the market maker.
     *
     * @param exchangeCode exchange code of this market maker event.
     */
    void setExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of this market maker event.
     * Returns the current market maker.
     *
     * @param exchangeCode exchange code of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withExchangeCode(char exchangeCode) noexcept;

    /**
     * Changes exchange code of this market maker event. It does not change the market maker.
     *
     * @param exchangeCode exchange code of this market maker event.
     */
    void setExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Changes exchange code of this market maker event.
     * Returns the current market maker.
     *
     * @param exchangeCode exchange code of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withExchangeCode(std::int16_t exchangeCode) noexcept;

    /**
     * Returns market maker or other aggregate identifier of this market maker event.
     *
     * @return market maker or other aggregate identifier of this market maker event, or dxfcpp::String::NUL
     * (`std::string{"<null>"}`) when it is not set.
     */
    std::string getMarketMaker() const;

    /**
     * Returns market maker or other aggregate identifier of this market maker event.
     *
     * @return market maker or other aggregate identifier of this market maker event, or `std::nullopt` when it is not
     * set (an empty string is not set as well).
     */
    std::optional<std::string> getMarketMakerOpt() const;

    /**
     * Changes market maker or other aggregate identifier of this market maker event. It does not change the exchange
     * code.
     *
     * The identifier is stored in the index as a QDS short string: at most 8 characters up to U+00FF, of which the
     * index keeps the last 4.
     *
     * @param marketMaker market maker or other aggregate identifier of this market maker event (UTF-8).
     * @throws InvalidArgumentException if the identifier is longer than 8 characters, has a character above U+00FF or
     * is not valid UTF-8.
     */
    void setMarketMaker(const StringLike &marketMaker);

    /**
     * Changes market maker or other aggregate identifier of this market maker event.
     * Returns the current market maker.
     *
     * @param marketMaker market maker or other aggregate identifier of this market maker event (UTF-8).
     * @return The current market maker.
     * @throws InvalidArgumentException if the identifier is longer than 8 characters, has a character above U+00FF or
     * is not valid UTF-8.
     */
    MarketMaker &withMarketMaker(const StringLike &marketMaker);

    /**
     * Returns time of the last bid change of this market maker event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid change.
     */
    std::int64_t getBidTime() const noexcept;

    /**
     * Changes time of the last bid change of this market maker event.
     *
     * @param bidTime time of the last bid change.
     */
    void setBidTime(std::int64_t bidTime) noexcept;

    /**
     * Changes time of the last bid change of this market maker event.
     * Returns the current market maker.
     *
     * @param bidTime time of the last bid change.
     * @return The current market maker.
     */
    MarketMaker &withBidTime(std::int64_t bidTime) noexcept;

    /**
     * Returns bid price of this market maker event.
     *
     * @return bid price of this market maker event.
     */
    double getBidPrice() const noexcept;

    /**
     * Changes bid price of this market maker event.
     *
     * @param bidPrice bid price of this market maker event.
     */
    void setBidPrice(double bidPrice) noexcept;

    /**
     * Changes bid price of this market maker event.
     * Returns the current market maker.
     *
     * @param bidPrice bid price of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withBidPrice(double bidPrice) noexcept;

    /**
     * Returns bid size of this market maker event.
     *
     * @return bid size of this market maker event.
     */
    double getBidSize() const noexcept;

    /**
     * Changes bid size of this market maker event.
     *
     * @param bidSize bid size of this market maker event.
     */
    void setBidSize(double bidSize) noexcept;

    /**
     * Changes bid size of this market maker event.
     * Returns the current market maker.
     *
     * @param bidSize bid size of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withBidSize(double bidSize) noexcept;

    /**
     * Returns number of individual bid orders in this market maker event.
     *
     * @return number of individual bid orders.
     */
    std::int64_t getBidCount() const noexcept;

    /**
     * Changes number of individual bid orders in this market maker event.
     *
     * @param bidCount number of individual bid orders.
     */
    void setBidCount(std::int64_t bidCount) noexcept;

    /**
     * Changes number of individual bid orders in this market maker event.
     * Returns the current market maker.
     *
     * @param bidCount number of individual bid orders.
     * @return The current market maker.
     */
    MarketMaker &withBidCount(std::int64_t bidCount) noexcept;

    /**
     * Returns time of the last ask change of this market maker event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last ask change.
     */
    std::int64_t getAskTime() const noexcept;

    /**
     * Changes time of the last ask change of this market maker event.
     *
     * @param askTime time of the last ask change.
     */
    void setAskTime(std::int64_t askTime) noexcept;

    /**
     * Changes time of the last ask change of this market maker event.
     * Returns the current market maker.
     *
     * @param askTime time of the last ask change.
     * @return The current market maker.
     */
    MarketMaker &withAskTime(std::int64_t askTime) noexcept;

    /**
     * Returns ask price of this market maker event.
     *
     * @return ask price of this market maker event.
     */
    double getAskPrice() const noexcept;

    /**
     * Changes ask price of this market maker event.
     *
     * @param askPrice ask price of this market maker event.
     */
    void setAskPrice(double askPrice) noexcept;

    /**
     * Changes ask price of this market maker event.
     * Returns the current market maker.
     *
     * @param askPrice ask price of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withAskPrice(double askPrice) noexcept;

    /**
     * Returns ask size of this market maker event.
     *
     * @return ask size of this market maker event.
     */
    double getAskSize() const noexcept;

    /**
     * Changes ask size of this market maker event.
     *
     * @param askSize ask size of this market maker event.
     */
    void setAskSize(double askSize) noexcept;

    /**
     * Changes ask size of this market maker event.
     * Returns the current market maker.
     *
     * @param askSize ask size of this market maker event.
     * @return The current market maker.
     */
    MarketMaker &withAskSize(double askSize) noexcept;

    /**
     * Returns number of individual ask orders in this market maker event.
     *
     * @return number of individual ask orders.
     */
    std::int64_t getAskCount() const noexcept;

    /**
     * Changes number of individual ask orders in this market maker event.
     *
     * @param askCount number of individual ask orders.
     */
    void setAskCount(std::int64_t askCount) noexcept;

    /**
     * Changes number of individual ask orders in this market maker event.
     * Returns the current market maker.
     *
     * @param askCount number of individual ask orders.
     * @return The current market maker.
     */
    MarketMaker &withAskCount(std::int64_t askCount) noexcept;

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
