// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>

#include "EventFlag.hpp"
#include "IndexedEventSource.hpp"

namespace dxfcpp {

/**
 * Represents an indexed collection of up-to-date information about some
 * condition or state of an external entity that updates in real-time. For example, ::Order represents an order to buy
 * or to sell some market instrument that is currently active on a market exchange and multiple
 * orders are active for each symbol at any given moment in time.
 * ::Candle represent snapshots of aggregate information about trading over a specific time period and there are
 * multiple periods available.
 * The ::Candle is also an example of ::TimeSeriesEvent that is a more specific event type.
 *
 * Index for each event is available via @ref ::getIndex() "index" property.
 * Indexed events retain information about multiple events per symbol based on the event index
 * and are conflated based on the event index. Last indexed event for each symbol and index is always
 * delivered to event listeners on subscription, but intermediate (next-to-last) events for each
 * symbol+index pair are not queued anywhere, they are simply discarded as stale events.
 * More recent events represent an up-to-date information about some external entity.
 *
 * Some indexed events, like ::Order, support multiple sources of information for the
 * same symbol. The event source is available via @ref IndexedEvent::getSource() "source" property.
 * The value of @ref IndexedEvent::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT"
 * for time-series events and other singe-sourced events like ::Series, that do not support this feature.
 *
 * See also the <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
 */
struct DXFCPP_EXPORT IndexedEvent {
    /// The alias to a type of shared pointer to the IndexedEvent object
    using Ptr = std::shared_ptr<IndexedEvent>;

    /// @copydoc EventFlag::TX_PENDING
    static const EventFlag TX_PENDING;

    /// @copydoc EventFlag::REMOVE_EVENT
    static const EventFlag REMOVE_EVENT;

    /// @copydoc EventFlag::SNAPSHOT_BEGIN
    static const EventFlag SNAPSHOT_BEGIN;

    /// @copydoc EventFlag::SNAPSHOT_END
    static const EventFlag SNAPSHOT_END;

    /// @copydoc EventFlag::SNAPSHOT_SNIP
    static const EventFlag SNAPSHOT_SNIP;

    /// @copydoc EventFlag::SNAPSHOT_MODE
    static const EventFlag SNAPSHOT_MODE;

    /**
     * Returns the source of this event.
     *
     * @return The source of this event.
     */
    virtual const IndexedEventSource &getSource() const & = 0;

    /**
     * Returns transactional event flags.
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     *
     * @return The transactional event flags.
     */
    virtual EventFlagsMask getEventFlags() const = 0;

    /**
     * Changes transactional event flags.
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     *
     * @param eventFlags transactional event flags.
     */
    virtual void setEventFlags(const EventFlagsMask &eventFlags) = 0;

    /**
     * Returns unique per-symbol index of this event.
     *
     * @return unique per-symbol index of this event.
     */
    virtual std::int64_t getIndex() const = 0;

    /**
     * Changes unique per-symbol index of this event.
     *
     * @param index unique per-symbol index of this event.
     */
    virtual void setIndex(std::int64_t index) = 0;
};

} // namespace dxfcpp