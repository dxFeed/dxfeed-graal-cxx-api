// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <string>
#include <cstdint>

#include "entity/SharedEntity.hpp"

namespace dxfcpp {

/**
 * Marks all event types that can be received via dxFeed API.
 * Events are considered instantaneous, non-persistent, and unconflateable
 * (each event is individually delivered) unless they implement one of interfaces
 * defined in this package to further refine their meaning.
 *
 * <p>Event types are POCO (plain old cpp objects) that follow bean naming convention with
 * getters and setters for their properties.
 * All event types are serializable, because they are transferred over network from publishers to
 * data feed consumers. However, they are using custom serialization format for this purpose.
 *
 * @see DXFeed
 */
struct EventType : public SharedEntity {
    /// The alias to a type of shared pointer to the EventType's child object.
    using Ptr = std::shared_ptr<EventType>;

    /**
     * Returns event symbol that identifies this event type in @ref DXFeedSubscription "subscription".
     *
     * @return The event symbol.
     */
    virtual const std::string &getEventSymbol() const = 0;

    /**
     * Changes event symbol that identifies this event type in @ref DXFeedSubscription "subscription".
     *
     * @param eventSymbol event symbol.
     */
    virtual void setEventSymbol(const std::string &eventSymbol) = 0;

    /**
     * Returns time when event was created or zero when time is not available.
     *
     * <p>This event time is available only when the corresponding DXEndpoint is created
     * with @ref DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY "DXENDPOINT_EVENT_TIME_PROPERTY" and
     * the data source has embedded event times. This is typically true only for data events
     * that are read from historical tape files and from OnDemandService.
     * Events that are coming from a network connections do not have an embedded event time information and
     * this method will return zero for them, meaning that event was received just now.
     *
     * Default implementation returns 0.
     *
     * @return The difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC or zero when time is not available.
     */
    virtual std::int64_t getEventTime() const { return 0; }

    /**
     * Changes event creation time.
     *
     * Default implementation does nothing.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     */
    virtual void setEventTime(std::int64_t eventTime){};
};

}