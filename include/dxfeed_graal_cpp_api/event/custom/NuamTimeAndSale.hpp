// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../market/TimeAndSale.hpp"

#include <cstdint>
#include <memory>
#include <string>

/**
 * \addtogroup dxfcpp_custom
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Represents an extension of TimeAndSale for the symbols traded on the Nuam Exchange.
 * It is the new Regional Holding that integrates the Santiago, Lima, and Colombia Stock Exchanges into a single market.
 *
 * It includes the Nuam Exchange internal data, obtained directly from the matching engine, such as different IDs.
 *
 * This is an experimental API, it may change in future versions.
 *
 * Java:
 * <a href="https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/custom/NuamTimeAndSale.html">NuamTimeAndSale</a>
 *
 * <h3>Properties</h3>
 *
 * NuamTimeAndSale event has all the properties of TimeAndSale and the following one:
 *
 * <ul>
 * <li>@ref NuamTimeAndSale::getMatchId() "matchId" - the unique identifier of the match associated with this
 * transaction.
 * </ul>
 *
 * <h3>Event flags, transactions and snapshots</h3>
 *
 * Some Nuam time and sale sources provide a consistent view of the set of known Nuam time and sales
 * for a given time range when used with DXFeedTimeSeriesSubscription.
 * The corresponding information is carried in @ref NuamTimeAndSale::getEventFlags() "eventFlags" property.
 * The logic behind this property is detailed in IndexedEvent class documentation.
 * Multiple event sources for the same symbol are not supported for Nuam time and sales, thus
 * @ref NuamTimeAndSale::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * <p>Regular subscription via DXFeedSubscription produces a stream of Nuam time and
 * sale events as they happen and their @ref NuamTimeAndSale::getEventFlags() "eventFlags" are always zero.
 *
 * <h3>Publishing Nuam Time and Sales</h3>
 *
 * Publishing of Nuam time and sales events follows the general rules explained in TimeSeriesEvent class
 * documentation.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `NuamTimeAndSale`.
 */
class DXFCPP_EXPORT NuamTimeAndSale final : public TimeAndSale {
    friend struct EventMapper;

    struct NuamTimeAndSaleData {
        std::int64_t matchId{};
    };

    NuamTimeAndSaleData nuamTimeAndSaleData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    public:
    /// The alias to a type of shared pointer to the NuamTimeAndSale object
    using Ptr = std::shared_ptr<NuamTimeAndSale>;

    /// The alias to a type of unique pointer to the NuamTimeAndSale object
    using Unique = std::unique_ptr<NuamTimeAndSale>;

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

    /// Creates new Nuam time and sale event with default values.
    NuamTimeAndSale() noexcept;

    /**
     * Creates new Nuam time and sale event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit NuamTimeAndSale(const StringLike &eventSymbol) noexcept;

    /**
     * Returns the match ID, which uniquely identifies the match between the buyer
     * and the seller associated with this transaction.
     * This information is available only to participants of the deal and broker.
     *
     * @return the match ID.
     */
    std::int64_t getMatchId() const noexcept;

    /**
     * Changes the match ID, which uniquely identifies the match between the buyer
     * and the seller associated with this transaction.
     * This information is available only to participants of the deal and broker.
     *
     * @param matchId the match ID to set.
     */
    void setMatchId(std::int64_t matchId) noexcept;

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
