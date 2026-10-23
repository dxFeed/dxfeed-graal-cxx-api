// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "./AuctionType.hpp"
#include "./ImbalanceSide.hpp"
#include "./MarketEvent.hpp"
#include "./OrderSource.hpp"

#include <cstdint>
#include <memory>
#include <string>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Order imbalance event provides information about order book statistics during auctions.
 *
 * These events describe the imbalance between buy and sell orders for specific security,
 * along with reference pricing information that reflects the current auction state. The event
 * includes metrics such as paired size, imbalance shares, and hypothetical pricing scenarios.
 *
 * This is an experimental API, it may change in future versions.
 *
 * <h3>Properties</h3>
 *
 * OrderImbalance event has the following properties:
 *
 * <ul>
 * <li>@ref OrderImbalance::getEventSymbol() "eventSymbol" - symbol of this event;
 * <li>@ref OrderImbalance::getSource() "source" - source of this event;
 * <li>@ref OrderImbalance::getTime() "time" - time of the order imbalance event;
 * <li>@ref OrderImbalance::getSequence() "sequence" - sequence of the order imbalance event;
 * <li>@ref OrderImbalance::getRefPrice() "refPrice" - reference price for imbalance calculation;
 * <li>@ref OrderImbalance::getPairedSize() "pairedSize" - total number of shares eligible to be matched at reference
 * price;
 * <li>@ref OrderImbalance::getImbalanceSize() "imbalanceSize" - number of shares not matched at reference price;
 * <li>@ref OrderImbalance::getNearPrice() "nearPrice" - hypothetical price for all interest on the book;
 * <li>@ref OrderImbalance::getFarPrice() "farPrice" - hypothetical price for auction interest only;
 * <li>@ref OrderImbalance::getImbalanceSide() "imbalanceSide" - side of the order imbalance;
 * <li>@ref OrderImbalance::getAuctionType() "auctionType" - type of auction.
 * </ul>
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `OrderImbalance#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "NUAM".
 */
class DXFCPP_EXPORT OrderImbalance final : public MarketEvent {
    friend struct EventMapper;

    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..6        5..3            2..0
     * +--------+--------------+----------------+
     * |        | Auction Type | Imbalance Side |
     * +--------+--------------+----------------+
     */

    // IMBALANCE_SIDE values are taken from the ImbalanceSide enum.
    static constexpr std::uint32_t IMBALANCE_SIDE_MASK = 0x07;
    static constexpr std::uint32_t IMBALANCE_SIDE_SHIFT = 0;

    // AUCTION_TYPE values are taken from the AuctionType enum.
    static constexpr std::uint32_t AUCTION_TYPE_MASK = 0x07;
    static constexpr std::uint32_t AUCTION_TYPE_SHIFT = 3;

    struct Data {
        std::int32_t sourceId{};
        std::int64_t timeSequence{};
        double refPrice = math::NaN;
        double pairedSize = math::NaN;
        double imbalanceSize = math::NaN;
        double nearPrice = math::NaN;
        double farPrice = math::NaN;
        std::int32_t flags{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    /// The alias to a type of shared pointer to the OrderImbalance object
    using Ptr = std::shared_ptr<OrderImbalance>;

    /// The alias to a type of unique pointer to the OrderImbalance object
    using Unique = std::unique_ptr<OrderImbalance>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Maximum allowed sequence value.
     *
     * @see OrderImbalance::setSequence()
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

    /// Creates new order imbalance event with default values.
    OrderImbalance() noexcept;

    /**
     * Creates a new order imbalance event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit OrderImbalance(const StringLike &eventSymbol) noexcept;

    // MarketEvent methods

    /**
     * Changes an event's symbol and returns the current order imbalance.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current order imbalance.
     */
    OrderImbalance &withEventSymbol(const StringLike &eventSymbol) noexcept;

    /**
     * Changes event's creation time and returns the current order imbalance.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current order imbalance.
     */
    OrderImbalance &withEventTime(std::int64_t eventTime) noexcept;

    // OrderImbalance methods

    /**
     * Returns source of this event.
     *
     * @return source of this event.
     */
    const OrderSource &getSource() const &;

    /**
     * Changes source of this event.
     *
     * @param source source of this event.
     */
    void setSource(const OrderSource &source) noexcept;

    /**
     * Changes source of this event.
     * Returns the current order imbalance.
     *
     * @param source source of this event.
     * @return The current order imbalance.
     */
    OrderImbalance &withSource(const OrderSource &source) noexcept;

    /**
     * Returns time and sequence of the order imbalance packaged into a single long value.
     *
     * @return time and sequence of the order imbalance.
     */
    std::int64_t getTimeSequence() const noexcept;

    /**
     * Changes time and sequence of the order imbalance.
     * <b>Do not use this method directly.</b>
     * Change @ref OrderImbalance::setTime() "time" and/or @ref OrderImbalance::setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see ::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Changes time and sequence of the order imbalance.
     * Returns the current order imbalance.
     *
     * @param timeSequence the time and sequence.
     * @return The current order imbalance.
     */
    OrderImbalance &withTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Returns time of the order imbalance event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the order imbalance event.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Changes time of the order imbalance event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of the order imbalance event.
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Changes time of the order imbalance event.
     * Returns the current order imbalance.
     *
     * @param time time of the order imbalance event.
     * @return The current order imbalance.
     */
    OrderImbalance &withTime(std::int64_t time) noexcept;

    /**
     * Returns sequence number of this event to distinguish order imbalance events that have the same
     * @ref OrderImbalance::getTime() "time". This sequence number does not have to be unique and
     * does not need to be sequential. Sequence can range from 0 to ::MAX_SEQUENCE.
     *
     * @return sequence of this order imbalance event.
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes @ref OrderImbalance::getSequence() "sequence number" of this event.
     *
     * @param sequence the sequence.
     * @throws InvalidArgumentException if the sequence is below zero or above ::MAX_SEQUENCE.
     * @see OrderImbalance::getSequence()
     */
    void setSequence(std::int32_t sequence);

    /**
     * Changes @ref OrderImbalance::getSequence() "sequence number" of this event.
     * Returns the current order imbalance.
     *
     * @param sequence the sequence.
     * @return The current order imbalance.
     * @throws InvalidArgumentException if the sequence is below zero or above ::MAX_SEQUENCE.
     * @see OrderImbalance::getSequence()
     */
    OrderImbalance &withSequence(std::int32_t sequence);

    /**
     * Returns the reference price.
     *
     * The reference price is the price for which the number of paired shares and number of
     * imbalance shares are calculated. Usually, this is the consolidated last sale price before
     * the auction or imbalance event started.
     *
     * @return the reference price.
     */
    double getRefPrice() const noexcept;

    /**
     * Changes the reference price.
     *
     * @param refPrice the reference price.
     */
    void setRefPrice(double refPrice) noexcept;

    /**
     * Changes the reference price.
     * Returns the current order imbalance.
     *
     * @param refPrice the reference price.
     * @return The current order imbalance.
     */
    OrderImbalance &withRefPrice(double refPrice) noexcept;

    /**
     * Returns the paired size.
     *
     * The paired size is the total number of shares that are eligible to be matched at the reference price.
     *
     * @return the paired size.
     */
    double getPairedSize() const noexcept;

    /**
     * Changes the paired size.
     *
     * @param pairedSize the paired size.
     */
    void setPairedSize(double pairedSize) noexcept;

    /**
     * Changes the paired size.
     * Returns the current order imbalance.
     *
     * @param pairedSize the paired size.
     * @return The current order imbalance.
     */
    OrderImbalance &withPairedSize(double pairedSize) noexcept;

    /**
     * Returns the imbalance size.
     *
     * The imbalance size is the number of shares that are not matched at the reference price.
     *
     * @return the imbalance size.
     */
    double getImbalanceSize() const noexcept;

    /**
     * Changes the imbalance size.
     *
     * @param imbalanceSize the imbalance size.
     */
    void setImbalanceSize(double imbalanceSize) noexcept;

    /**
     * Changes the imbalance size.
     * Returns the current order imbalance.
     *
     * @param imbalanceSize the imbalance size.
     * @return The current order imbalance.
     */
    OrderImbalance &withImbalanceSize(double imbalanceSize) noexcept;

    /**
     * Returns the near price.
     *
     * The near price is a hypothetical price at which all interest (all orders) on the book
     * could trade if the closure happened now, including auction and imbalance offset interest.
     *
     * @return the near price.
     */
    double getNearPrice() const noexcept;

    /**
     * Changes the near price.
     *
     * @param nearPrice the near price.
     */
    void setNearPrice(double nearPrice) noexcept;

    /**
     * Changes the near price.
     * Returns the current order imbalance.
     *
     * @param nearPrice the near price.
     * @return The current order imbalance.
     */
    OrderImbalance &withNearPrice(double nearPrice) noexcept;

    /**
     * Returns the far price.
     *
     * The far price is a hypothetical price at which only auction interest (all auction orders)
     * on the book could trade if the closure happened now. Far price is usually further from the
     * reference price than the near price.
     *
     * @return the far price.
     */
    double getFarPrice() const noexcept;

    /**
     * Changes the far price.
     *
     * @param farPrice the far price.
     */
    void setFarPrice(double farPrice) noexcept;

    /**
     * Changes the far price.
     * Returns the current order imbalance.
     *
     * @param farPrice the far price.
     * @return The current order imbalance.
     */
    OrderImbalance &withFarPrice(double farPrice) noexcept;

    /**
     * Returns the imbalance side.
     *
     * @return the imbalance side.
     */
    const ImbalanceSide &getImbalanceSide() const & noexcept;

    /**
     * Changes the imbalance side.
     *
     * @param side the imbalance side.
     */
    void setImbalanceSide(const ImbalanceSide &side) noexcept;

    /**
     * Changes the imbalance side.
     * Returns the current order imbalance.
     *
     * @param side the imbalance side.
     * @return The current order imbalance.
     */
    OrderImbalance &withImbalanceSide(const ImbalanceSide &side) noexcept;

    /**
     * Returns the auction type.
     *
     * @return the auction type.
     */
    const AuctionType &getAuctionType() const & noexcept;

    /**
     * Changes the auction type.
     *
     * @param type the auction type.
     */
    void setAuctionType(const AuctionType &type) noexcept;

    /**
     * Changes the auction type.
     * Returns the current order imbalance.
     *
     * @param type the auction type.
     * @return The current order imbalance.
     */
    OrderImbalance &withAuctionType(const AuctionType &type) noexcept;

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
