// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../symbols/SymbolWrapper.hpp"
#include "../FilteredSubscriptionSymbol.hpp"

#include <cstdint>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct SymbolWrapper;
class IndexedEventSubscriptionSymbol;

/**
 * Represents subscription to time-series of events.
 * This is symbol is observed by ObservableSubscriptionChangeListener
 * methods @ref ObservableSubscriptionChangeListener::symbolsAdded() "symbolsAdded"
 * and @ref ObservableSubscriptionChangeListener::symbolsRemoved() "symbolsRemoved"
 * when time-series subscription is created via DXFeedTimeSeriesSubscription class.
 *
 * <p>Instances of this class can be used with DXFeedSubscription to specify subscription
 * for time series events from a specific time. By default, subscribing to time-series events by
 * their event symbol object, the subscription is performed to a stream of new events as they happen only.
 *
 * <p>TimeSeriesEvent represents a special subtype of IndexedEvent.
 * The source identifier of a time-series event is always zero and thus
 *
 * <h3>Equality and hash codes</h3>
 *
 * This is a FilteredSubscriptionSymbol.
 * Time-series subscription symbols are compared based on their @ref TimeSeriesSubscriptionSymbol::getEventSymbol()
 * "eventSymbol" only. It means, that a set of time-series subscription symbols can contain at most one time-series
 * subscription for each event symbol.
 */
class DXFCPP_EXPORT TimeSeriesSubscriptionSymbol final : public IndexedEventSubscriptionSymbol,
                                                         public FilteredSubscriptionSymbol {
    std::int64_t fromTime_{};

  public:
    /**
     * Creates time-series subscription symbol with a specified event symbol and subscription time.
     * @param eventSymbol the wrapped event symbol (CandleSymbol, WildcardSymbol, etc).
     * @param fromTime the subscription time.
     */
    TimeSeriesSubscriptionSymbol(const SymbolWrapper &eventSymbol, std::int64_t fromTime);

    TimeSeriesSubscriptionSymbol(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol);

    TimeSeriesSubscriptionSymbol(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept;

    TimeSeriesSubscriptionSymbol &operator=(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol);

    TimeSeriesSubscriptionSymbol &operator=(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept;

    TimeSeriesSubscriptionSymbol() noexcept = default;

    ~TimeSeriesSubscriptionSymbol() noexcept override = default;

    /**
     * Returns the subscription time.
     *
     * @return the subscription time.
     */
    std::int64_t getFromTime() const;

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

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure (recursively
     * if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static TimeSeriesSubscriptionSymbol fromGraal(void *graalNative);

    /**
     * Returns string representation of this time-series subscription symbol.
     *
     * @return string representation of this time-series subscription symbol.
     */
    std::string toString() const override;

    bool operator==(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept;

    bool operator<(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept;
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::TimeSeriesSubscriptionSymbol> {
    std::size_t operator()(const dxfcpp::TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
        return std::hash<std::unique_ptr<dxfcpp::SymbolWrapper>>{}(timeSeriesSubscriptionSymbol.getEventSymbol());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()