// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>

#include "IndexedEvent.hpp"
#include "IndexedEventSource.hpp"

namespace dxfcpp {

/**
 * Represents time-series snapshots of some process that is evolving in time or actual events in some external system
 * that have an associated time stamp and can be uniquely identified.
 * For example, TimeAndSale events represent the actual sales that happen on a market exchange at specific time
 * moments, while Candle events represent snapshots of aggregate information about trading over a specific time period.
 *
 * Time-series events can be used with DXFeedTimeSeriesSubscription to receive a time-series history of past events.
 * Time-series events are conflated based on unique @ref ::getIndex() "index" for each symbol.
 * Last indexed event for each symbol and index is always delivered to event listeners on subscription, but
 * intermediate (next-to-last) events for each symbol+index pair are not queued anywhere, they are simply discarded as
 * stale events.
 *
 * Timestamp of an event is available via @ref ::getTime() "time" property with a millisecond precision. Some events
 * may happen multiple times per millisecond. In this case they have the same @ref ::getTime() "time", but different
 * @ref ::getIndex() "index". An ordering of @ref ::getIndex() "index" is the same as an ordering
 * of @ref ::getTime() "time". That is, an collection of time-series events that is ordered
 * by @ref ::getIndex() "index" is ascending order will be also ordered by @ref ::getTime() "time" in ascending order.
 *
 * Time-series events are a more specific subtype of IndexedEvent.
 * All general documentation and <a href="IndexedEvent.html#eventFlagsSection">Event Flags</a> section, in particular,
 * applies to time-series events. However, the time-series events never come from multiple sources for the same symbol
 * and their @ref #getSource() "source" is always @ref IndexedEventSource::DEFAULT "DEFAULT".
 *
 * Unlike a general IndexedEvent that is subscribed to via DXFeedSubscription} using a plain symbol to receive all
 * events for all indices, time-series events are typically subscribed to using TimeSeriesSubscriptionSymbol class to
 * specific time range of the subscription. There is a dedicated DXFeedTimeSeriesSubscription class that is designed
 * to simplify the task of subscribing for time-series events.
 *
 * TimeSeriesEventModel class handles all the snapshot and transaction logic and conveniently represents a list of
 * current time-series events ordered by their @ref ::getTime() "time". It relies on the code
 * of AbstractIndexedEventModel to handle this logic.
 * Use the source code of AbstractIndexedEventModel for clarification on transactions and snapshot logic.
 *
 * Classes that implement this interface may also implement LastingEvent interface, which makes it possible to use
 * DXFeed::getLastEvent() method to retrieve the last event for the corresponding symbol.
 *
 * <h3>Publishing time-series</h3>
 *
 * When publishing time-series event with DXPublisher::publishEvents() method on incoming TimeSeriesSubscriptionSymbol
 * the snapshot of currently known events for the requested time range has to be published first.
 *
 * A snapshot is published in the <em>descending</em> order of @ref ::getIndex() "index" (which is the same as the
 * descending order of @ref ::getTime() "time"), starting with an event with the largest index and marking it
 * with ::SNAPSHOT_BEGIN bit in @ref ::getEventFlags "eventFlags". All other event follow with default, zero
 * @ref ::getEventFlags() "eventFlags". If there is no actual event at the time that was specified in subscription
 * @ref TimeSeriesSubscriptionSymbol::getFromTime() "fromTime" property, then event with the corresponding time
 * has to be created anyway and published. To distinguish it from the actual event, it has to be marked with
 * ::REMOVE_EVENT bit in @ref ::getEventFlags() "eventFlags". ::SNAPSHOT_END bit in this event's
 * @ref #getEventFlags() "eventFlags" s optional during publishing. It will be properly set on receiving end anyway.
 * Note, that publishing any event with time that is below subscription
 * @ref TimeSeriesSubscriptionSymbol::getFromTime() "fromTime" also works as a legal indicator for the end of the
 * snapshot.
 *
 * Both TimeAndSale and Candle time-series events define a sequence property that is mixed into an
 * @ref ::getIndex() "index" property. It provides a way to distinguish different events at the same time. For a
 * snapshot end event the sequence has to be left at its default zero value. It means, that if there is an actual
 * event to be published at subscription @ref TimeSeriesSubscriptionSymbol::getFromTime() "fromTime" with non-zero
 * sequence, then generation of an additional snapshot end event with the subscription
 * @ref TimeSeriesSubscriptionSymbol::getFromTime() "fromTime" and zero sequence is still required.
 */
struct DXFCPP_EXPORT TimeSeriesEvent : public IndexedEvent {
    /// The alias to a type of shared pointer to the TimeSeriesEvent object
    using Ptr = std::shared_ptr<TimeSeriesEvent>;

    /**
     * Returns the source identifier for this event, which is always @ref IndexedEventSource::DEFAULT "DEFAULT" for
     * time-series events.
     *
     * @return The source identifier for this event, which is always @ref IndexedEventSource::DEFAULT "DEFAULT" for
     * time-series events.
     */
    const IndexedEventSource &getSource() const & override { return IndexedEventSource::DEFAULT; }

    /**
     * Returns unique per-symbol index of this event.
     * Event indices are unique within event symbol.
     * Typically event index for a time series event includes @ref ::getTime() "time" inside.
     *
     * <h3>Implementation notes</h3>
     *
     * The most common scheme for event indices is to set highest 32 bits of event index
     * to event timestamp in seconds. The lowest 32 bits are then split as follows.
     * Bits 22 to 31 encode milliseconds of time stamp, and bits 0 to 21 encode some
     * kind of a sequence number.
     *
     * Ultimately, the scheme for event indices is specific for each even type.
     * The actual classes for specific event types perform the corresponding encoding.
     *
     * @return unique per-symbol index of this event.
     */
    std::int64_t getIndex() const override { return 0; }

    /**
     * Returns unique per-symbol index of this event.
     *
     * @return unique per-symbol index of this event.
     * @deprecated Use ::getIndex()
     */
    virtual std::int64_t getEventId() const { return getIndex(); }

    /**
     * Returns timestamp of the event.
     * The timestamp is in milliseconds from midnight, January 1, 1970 UTC.
     *
     * @return timestamp of the event.
     */
    virtual std::int64_t getTime() const = 0;
};

} // namespace dxfcpp