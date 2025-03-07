// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <string>

#include "../entity/EntityModule.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * Marks all event types that can be received via dxFeed API.
 * Events are considered instantaneous, non-persistent, and unconflateable
 * (each event is individually delivered) unless they implement one of interfaces
 * defined in this package to further refine their meaning.
 *
 * <p>Event types are POCO (plain old cpp objects, not POD) that follow bean naming convention with
 * getters and setters for their properties.
 * All event types are serializable, because they are transferred over network from publishers to
 * data feed consumers. However, they are using custom serialization format for this purpose.
 *
 * @see DXFeed
 */
struct DXFCPP_EXPORT EventType : public SharedEntity {
    /// The alias to a type of shared pointer to the EventType object.
    using Ptr = std::shared_ptr<EventType>;

    EventType();

    ~EventType() noexcept override;

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
    virtual std::int64_t getEventTime() const noexcept {
        return 0;
    }

    /**
     * Changes event creation time.
     *
     * Default implementation does nothing.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     */
    virtual void setEventTime(std::int64_t /*eventTime*/) noexcept {
        // The default implementation is empty
    };

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    virtual void *toGraal() const = 0;

    /**
     * Replaces the contents of the event.
     *
     * @param event the event to use as source.
     */
    virtual void assign(std::shared_ptr<EventType> event) {
        ignoreUnused(event);
    }

    ///
    std::string toString() const override {
        return "EventType{}";
    }

    friend std::ostream &operator<<(std::ostream &os, const EventType &e) {
        return os << e.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const std::shared_ptr<EventType> &e) {
        return os << e->toString();
    }

    template <typename EntityType>
    friend std::ostream &operator<<(std::ostream &os, const std::shared_ptr<EntityType> &e)
#if __cpp_concepts
        requires(std::is_base_of_v<EventType, EntityType>)
#endif
    {
        return os << e->toString();
    }
};

/**
 * Event type parametrized by a symbol
 *
 * @tparam Symbol The type od symbol
 */
template <typename Symbol> struct DXFCPP_EXPORT EventTypeWithSymbol : public EventType {
    /// The alias to a type of shared pointer to the EventTypeWithSymbol's child object.
    using Ptr = std::shared_ptr<EventTypeWithSymbol<Symbol>>;

    using SymbolType = Symbol;

    /**
     * Returns event symbol that identifies this event type in @ref DXFeedSubscription "subscription".
     *
     * @return The event symbol.
     */
    virtual const Symbol &getEventSymbol() const & noexcept = 0;

    /**
     * Returns event symbol that identifies this event type in @ref DXFeedSubscription "subscription".
     *
     * @return The event symbol or std::nullopt.
     */
    virtual const std::optional<Symbol> &getEventSymbolOpt() const & noexcept = 0;

    /**
     * Changes event symbol that identifies this event type in @ref DXFeedSubscription "subscription".
     *
     * @param eventSymbol event symbol.
     */
    virtual void setEventSymbol(const Symbol &eventSymbol) noexcept = 0;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()