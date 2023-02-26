// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace dxfcpp {

/**
 * The enumeration type that provides additional information about the dxFeed C++-API event type.
 * Useful when used as a subscription parameter.
 */
class EventTypeEnum {
    /// The dxFeed Graal Native C-API event class id corresponding to the current enum element.
    std::uint32_t id_;

    std::string name_;

    // A flag that indicates that the current enum element is characterizing the Lasting (TICKER) event.
    bool isLasting_;

    // A flag that indicates that the current enum element is characterizing the Indexed event.
    bool isIndexed_;

    // A flag that indicates that the current enum element is characterizing the TimeSeries (HISTORY) event.
    bool isTimeSeries_;

    // A flag that indicates that the current enum element is characterizing the OnlyIndexed (Indexed, but not
    // TimeSeries) event.
    bool isOnlyIndexed_;

    EventTypeEnum(std::uint32_t id, std::string name, bool isLasting, bool isIndexed = false, bool isTimeSeries = false)
        : id_{id}, name_{std::move(name)}, isLasting_{isLasting}, isIndexed_{isIndexed || isTimeSeries},
          isTimeSeries_{isTimeSeries}, isOnlyIndexed_{isIndexed && !isTimeSeries} {}

  public:
    static const EventTypeEnum QUOTE;
    static const EventTypeEnum PROFILE;
    static const EventTypeEnum SUMMARY;
    static const EventTypeEnum GREEKS;
    static const EventTypeEnum CANDLE;

    // static const EventTypeEnum DAILY_CANDLE; // deprecated

    static const EventTypeEnum UNDERLYING;
    static const EventTypeEnum THEO_PRICE;
    static const EventTypeEnum TRADE;
    static const EventTypeEnum TRADE_ETH;
    static const EventTypeEnum CONFIGURATION;
    static const EventTypeEnum MESSAGE;
    static const EventTypeEnum TIME_AND_SALE;

    // static const EventTypeEnum ORDER_BASE; // non abstract, unusable

    static const EventTypeEnum ORDER;
    static const EventTypeEnum ANALYTIC_ORDER;
    static const EventTypeEnum SPREAD_ORDER;
    static const EventTypeEnum SERIES;
    static const EventTypeEnum OPTION_SALE;

    explicit EventTypeEnum() : EventTypeEnum{static_cast<std::uint32_t>(-1), "INVALID", false} {}

    /**
     * @return The dxFeed Graal Native C-API event class id
     */
    std::uint32_t getId() const { return id_; }

    /**
     * @return The current enum element name
     */
    const std::string &getName() const { return name_; }

    bool operator==(const EventTypeEnum &eventTypeEnum) const { return id_ == eventTypeEnum.id_; }

    bool operator<(const EventTypeEnum &eventTypeEnum) const { return id_ < eventTypeEnum.id_; }

    /**
     * @return `true` if the current enum element is characterizing the Lasting (TICKER) event
     */
    bool isLasting() const { return isLasting_; }

    /**
     * @return `true` if the current enum element is characterizing the Indexed event.
     */
    bool isIndexed() const { return isIndexed_; }

    /**
     * @return `true` if the current enum element is characterizing the TimeSeries (HISTORY) event.
     */
    bool isTimeSeries() const { return isTimeSeries_; }

    /**
     * @return `true` if the current enum element is characterizing the OnlyIndexed (Indexed, but not TimeSeries) event.
     */
    bool isOnlyIndexed() const { return isOnlyIndexed_; }
};

} // namespace dxfcpp

namespace std {

template <> struct hash<dxfcpp::EventTypeEnum> {
    std::size_t operator()(const dxfcpp::EventTypeEnum &eventType) const noexcept { return eventType.getId(); }
};

} // namespace std

namespace dxfcpp {

/// Base abstract class for all dxFeed C++ API entities
struct Entity {
    /// The default virtual d-tor
    virtual ~Entity() = default;
};

/// Base abstract "shared entity" class. Has some helpers for dynamic polymorphism
struct SharedEntity : public Entity, std::enable_shared_from_this<SharedEntity> {
    /// The simple type synonym for the SharedEntity type
    using Ptr = std::shared_ptr<SharedEntity>;

    /**
     * Checks that pointer to the current type could be converted to type T*
     * In other words: whether type T belongs to the type hierarchy in which the current type resides.
     * @tparam T The type to check
     * @return true if type belongs to the type hierarchy in which the current type resides.
     */
    template <typename T> bool is() const noexcept {
        try {
            auto p = dynamic_cast<const T *>(this);
            (void)(p);

            return true;
        } catch (const std::bad_cast &) {
            return false;
        }
    }

    /**
     *
     * @tparam T
     * @return
     */
    template <typename T> std::shared_ptr<T> sharedAs() noexcept {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    /**
     *
     * @tparam T
     * @return
     */
    template <typename T> std::shared_ptr<T> sharedAs() const {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }
};

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

/**
 * Base class for all market events. All market events are POCO that
 * extend this class. Market event classes are simple beans with setter and getter methods for their
 * properties and minimal business logic. All market events have `eventSymbol` property that is
 * defined by this class.
 */
struct MarketEvent : public EventType {
    /// The alias to a type of shared pointer to the MarketEvent object
    using Ptr = std::shared_ptr<MarketEvent>;

  private:
    std::string eventSymbol_{};
    std::int64_t eventTime_{};

  protected:
    MarketEvent() noexcept {}

    /**
     * Protected constructor for concrete implementation classes that initializes `eventSymbol` property.
     *
     * @param eventSymbol The event symbol.
     */
    explicit MarketEvent(std::string eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {}

  public:
    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event.
     */
    const std::string &getEventSymbol() const override { return eventSymbol_; }

    /**
     * Changes symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const std::string &eventSymbol) override { eventSymbol_ = eventSymbol; }

    ///
    std::int64_t getEventTime() const override { return eventTime_; }

    ///
    void setEventTime(std::int64_t eventTime) override { eventTime_ = eventTime; }
};

/**
 * Source identifier for IndexedEvent.
 *
 * See IndexedEvent::getSource().
 */
class IndexedEventSource {
    std::uint32_t id_{};
    std::string name_{};

  public:
    /**
     * The default source with zero @ref ::id() "identifier" for all events that do not support multiple sources.
     */
    static const IndexedEventSource DEFAULT;

    /**
     * Creates the new IndexedEvent's source by id and name.
     *
     * @param id The source id
     * @param name The source name
     */
    IndexedEventSource(unsigned id, std::string name) : id_{id}, name_{std::move(name)} {}

    /**
     * Returns the source identifier. Source identifier is non-negative.
     *
     * @return The source identifier.
     */
    std::uint32_t id() const { return id_; }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    const std::string &name() const { return name_; }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    std::string toString() const { return name_; }
};

class EventFlagsMask;

/**
 * <h3><a name="eventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Some indexed event sources provide a consistent view of a set of events for a given symbol. Their updates
 * may incorporate multiple changes that have to be processed at the same time.
 * The corresponding information is carried in @ref IndexedEvent::getEventFlags() "eventFlags" property.
 *
 * <p> The value of @ref IndexedEvent::getEventFlags() "eventFlags" property has several significant bits that are
 * packed into an integer in the following way:
 *
 * <pre><tt>
 *    31..7    6    5    4    3    2    1    0
 * +---------+----+----+----+----+----+----+----+
 * |         | SM |    | SS | SE | SB | RE | TX |
 * +---------+----+----+----+----+----+----+----+
 * </tt></pre>
 *
 * Each source updates its transactional state using these bits separately.
 * The state of each source has to be tracked separately in a map for each source.
 * However, event @ref IndexedEvent::getIndex() "index" is unique across the sources. This is achieved by allocating
 * an event-specific number of most significant bits of @ref IndexedEvent::getIndex() "index" for use as
 * a @ref IndexedEvent::getSource() "source" @ref IndexedEventSource::id() "id".
 *
 * <p> `TX` (bit 0) &mdash; ::TX_PENDING is an indicator of pending transactional update.
 * It can be retrieved from `eventFlags` with the following piece of code:
 *
 * ```cpp
 * bool txPending = (event-> getEventFlags() & IndexedEvent::TX_PENDING) != 0;
 * ```
 *
 * <p>When `txPending` is `true` it means, that an ongoing transaction update that spans multiple events is
 * in process. All events with `txPending` `true` shall be put into a separate <em>pending list</em>
 * for each source id and should be processed later when an event for this source id with `txPending` `false` comes.
 *
 * <p> `RE` (bit 1) &mdash; ::REMOVE_EVENT is used to indicate that that the event with the
 * corresponding index has to be removed.
 *
 * ```cpp
 * bool removeEvent = (event-> getEventFlags() & IndexedEvent::REMOVE_EVENT) != 0;
 * ```
 *
 * <p> `SB` (bit 2) &mdash; ::SNAPSHOT_BEGIN is used to indicate when the loading of a snapshot starts.
 *
 * ```cpp
 * bool snapshotBegin = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_BEGIN) != 0;
 * ```
 *
 * <p> Snapshot load starts on new subscription and the first indexed event that arrives for each non-zero source id
 * on new subscription may have `snapshotBegin` set to `true`. It means, that an ongoing snapshot
 * consisting of multiple events is incoming. All events for this source id shall be put into a separate
 * <em>pending list</em> for each source id.
 *
 * <p> `SE` (bit 3) &mdash; ::SNAPSHOT_END or `SS` (bit 4) &mdash; ::SNAPSHOT_SNIP are used to indicate the end of a
 * snapshot.
 *
 * ```cpp
 * bool snapshotEnd = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_END) != 0;
 * bool snapshotSnip = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_SNIP) != 0;
 * ```
 *
 * <p>The last event of a snapshot is marked with either `snapshotEnd` or `snapshotSnip`. At this time, all events
 * from a <em>pending list</em> for the corresponding source can be processed, unless `txPending` is also
 * set to `true`. In the later case, the processing shall be further delayed due to ongoing transaction.
 *
 * <p>The difference between `snapshotEnd` and `snapshotSnip` is the following.
 * `snapshotEnd` indicates that the data source had sent all the data pertaining to the subscription
 * for the corresponding indexed event, while `snapshotSnip` indicates that some limit on the amount
 * of data was reached and while there still might be more data available, it will not be provided.
 *
 * <p> `SM` (bit 6) &mdash; ::SNAPSHOT_MODE is used to instruct dxFeed to use snapshot mode.
 * It is intended to be used only for publishing to activate (if not yet activated) snapshot mode.
 * The difference from ::SNAPSHOT_BEGIN flag is that ::SNAPSHOT_MODE only switches on snapshot mode
 * without starting snapshot synchronization protocol.
 *
 * <p> When a snapshot is empty or consists of a single event, then the event can have both `snapshotBegin`
 * and `snapshotEnd` or `snapshotSnip` flags. In case of an empty snapshot, `removeEvent` on this event is
 * also set to `true`.
 */
class EventFlag final {
    std::uint32_t flag_;
    std::string name_;

    EventFlag(std::uint32_t flag, std::string name) : flag_{flag}, name_{std::move(name)} {}

  public:
    /**
     * `0x01` - A bitmask to get transaction pending indicator from the value of @ref ::getEventFlags() "eventFlags"
     * property.
     *
     * ```cpp
     * bool txPending = (event-> getEventFlags() & IndexedEvent::TX_PENDING) != 0;
     * ```
     *
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag TX_PENDING;

    /**
     * `0x02` - A bitmask to get removal indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * ```cpp
     * bool removeEvent = (event-> getEventFlags() & IndexedEvent::REMOVE_EVENT) != 0;
     * ```
     *
     * See <a href="#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag REMOVE_EVENT;

    /**
     * `0x04` - A bitmask to get snapshot begin indicator from the value of @ref ::getEventFlags() "eventFlags"
     * property.
     *
     * ```cpp
     * bool snapshotBegin = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_BEGIN) != 0;
     * ```
     *
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag SNAPSHOT_BEGIN;

    /**
     * `0x08` - A bitmask to get snapshot end indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * ```cpp
     * bool snapshotEnd = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_END) != 0;
     * ```
     *
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag SNAPSHOT_END;

    /**
     * `0x10` - A bitmask to get snapshot snip indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * ```cpp
     * bool snapshotSnip = (event-> getEventFlags() & IndexedEvent::SNAPSHOT_SNIP) != 0;
     * ```
     *
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag SNAPSHOT_SNIP;

    // 0x20 is reserved. This flag will fit into 1-byte on the wire in QTP protocol

    /**
     * `0x40` - A bitmask to set snapshot mode indicator into the value of @ref ::setEventFlags() "eventFlags" property.
     * This flag is intended for publishing only.
     *
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     */
    static const EventFlag SNAPSHOT_MODE;

    /**
     * `0x80` - For internal use. Marks a subscription for deletion.
     */
    static const EventFlag REMOVE_SYMBOL;

    /**
     * Creates the invalid event flag
     */
    explicit EventFlag() : flag_{unsigned(-1)}, name_{"INVALID"} {}

    /**
     * @return The event flag's value
     */
    std::uint32_t getFlag() const { return flag_; }

    /**
     * Determines if the given flag is in the mask.
     *
     * @param eventFlagsMask The event flags mask
     *
     * @return `true` the given flag is in the mask.
     */
    bool in(std::uint32_t eventFlagsMask) const { return (eventFlagsMask & flag_) != 0; }

    /**
     * Determines if the given flag is in the mask.
     *
     * @tparam EventFlagsMask An event flags mask type that satisfies the condition: there is a `getMask` method that
     * returns std::uint32_t
     * @param eventFlagsMask The event flags mask.
     * @return `true` the given flag is in the mask.
     */
    template <typename EventFlagsMask>
    bool in(const EventFlagsMask &eventFlagsMask) const
        requires requires {
                     { eventFlagsMask.getMask() } -> std::same_as<std::uint32_t>;
                 }
    {
        return in(eventFlagsMask.getMask());
    }

    ///
    const std::string &getName() const { return name_; }

    ///
    std::string toString() const { return name_; }
};

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
struct IndexedEvent {
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
    virtual const IndexedEventSource &getSource() const = 0;

    /**
     * Returns transactional event flags.
     * See <a href="./de/d03/classdxfcpp_1_1_event_flag.html#eventFlagsSection">"Event Flags" section</a>
     *
     * @return The transactional event flags.
     */
    virtual const EventFlagsMask &getEventFlags() const = 0;

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
    virtual std::uint64_t getIndex() const = 0;

    /**
     * Changes unique per-symbol index of this event.
     *
     * @param index unique per-symbol index of this event.
     */
    virtual void setIndex(std::uint64_t index) = 0;
};

/**
 * Represents up-to-date information about some condition or state of an external entity that updates in real-time.
 * For example, a Quote is an up-to-date information about best bid and best offer for a specific symbol.
 *
 * Lasting events are conflated for each symbol. Last event for each symbol is always delivered to event listeners on
 * subscription, but intermediate (next-to-last) events are not queued anywhere, they are simply discarded as stale
 * events. More recent events represent an up-to-date information about some external entity.
 *
 * Lasting events can be used with DXFeed::getLastEvent() and DXFeed::getLastEvents methods to retrieve last events
 * for each symbol.
 *
 * Note, that subscription to all lasting events of a specific type via WildcardSymbol::ALL symbol object does not
 * benefit from the above advantages of lasting events.
 */
struct LastingEvent {
    /// The alias to a type of shared pointer to the LastingEvent object
    using Ptr = std::shared_ptr<LastingEvent>;
};

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
struct TimeSeriesEvent : public IndexedEvent {
    /// The alias to a type of shared pointer to the TimeSeriesEvent object
    using Ptr = std::shared_ptr<TimeSeriesEvent>;

    /**
     * Returns the source identifier for this event, which is always @ref IndexedEventSource::DEFAULT "DEFAULT" for
     * time-series events.
     *
     * @return The source identifier for this event, which is always @ref IndexedEventSource::DEFAULT "DEFAULT" for
     * time-series events.
     */
    const IndexedEventSource &getSource() const override;

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
    std::uint64_t getIndex() const override;

    /**
     * Returns unique per-symbol index of this event.
     *
     * @return unique per-symbol index of this event.
     * @deprecated Use ::getIndex()
     */
    virtual std::uint64_t getEventId() const { return getIndex(); }

    /**
     * Returns timestamp of the event.
     * The timestamp is in milliseconds from midnight, January 1, 1970 UTC.
     *
     * @return timestamp of the event.
     */
    virtual std::uint64_t getTime() const = 0;
};

/**
 * Quote event is a snapshot of the best bid and ask prices, and other fields that change with each quote.
 * It represents the most recent information that is available about the best quote on the market
 * at any given moment of time.
 */
class Quote final : public MarketEvent, public LastingEvent {
    /**
     * Maximum allowed sequence value.
     *
     * @see ::setSequence()
     */
    static const std::int32_t MAX_SEQUENCE = (1 << 22) - 1;

    struct Data {
        std::int32_t timeMillisSequence{};
        std::int32_t timeNanoPart{};
        std::int64_t bidTime{};
        std::int16_t bidExchangeCode{};
        double bidPrice = detail::math::NaN;
        double bidSize = detail::math::NaN;
        std::int64_t askTime{};
        std::int16_t askExchangeCode{};
        double askPrice = detail::math::NaN;
        double askSize = detail::math::NaN;
    } data_;

    void recomputeTimeMillisPart() {
        data_.timeMillisSequence = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(detail::time_util::getMillisFromTime(std::max(data_.askTime, data_.bidTime)))
                << 22U |
            static_cast<std::uint32_t>(getSequence()));
    }

  public:
    static const EventTypeEnum type;

    /// Creates new quote with default values.
    Quote() noexcept : data_{} {}

    /**
     * Creates new quote with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Quote(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)), data_{} {}

    /**
     * Returns sequence number of this quote to distinguish quotes that have the same @ref ::getTime() "time". This
     * sequence number does not have to be unique and does not need to be sequential. Sequence can range from 0 to
     * ::MAX_SEQUENCE.
     *
     * @return sequence of this quote.
     */
    std::int32_t getSequence() const {
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(data_.timeMillisSequence) &
                                         static_cast<std::uint32_t>(MAX_SEQUENCE));
    }

    /**
     * Changes @ref ::getSequence() "sequence number" of this quote.
     *
     * @param sequence The sequence.
     *
     * @see ::getSequence()
     */
    void setSequence(std::int32_t sequence) {
        assert(sequence >= 0 && sequence <= MAX_SEQUENCE);

        data_.timeMillisSequence = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(data_.timeMillisSequence) & ~static_cast<std::uint32_t>(MAX_SEQUENCE)) |
            static_cast<std::uint32_t>(sequence));
    }

    /**
     * Returns time of the last bid or ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid or ask change.
     */
    std::int64_t getTime() const {
        return detail::math::floorDiv(std::max(data_.bidTime, data_.askTime), 1000) * 1000 +
               static_cast<std::int64_t>(static_cast<std::uint32_t>(data_.timeMillisSequence) >> 22U);
    }

    /**
     * Returns time of the last bid or ask change in nanoseconds.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the last bid or ask change in nanoseconds.
     */
    std::int64_t getTimeNanos() const {
        return detail::time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
    }

    /**
     * Returns microseconds and nanoseconds part of time of the last bid or ask change.
     *
     * @return microseconds and nanoseconds part of time of the last bid or ask change.
     */
    std::int32_t getTimeNanoPart() const { return data_.timeNanoPart; }

    /**
     * Changes microseconds and nanoseconds part of time of the last bid or ask change.
     * <b>This method changes ::getTimeNanos() result.</b>
     *
     * @param timeNanoPart The microseconds and nanoseconds part of time of the last bid or ask change.
     */
    void setTimeNanoPart(int32_t timeNanoPart) { data_.timeNanoPart = timeNanoPart; }

    /**
     * Returns time of the last bid change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return time of the last bid change.
     */
    std::int64_t getBidTime() const { return data_.bidTime; }

    /**
     * Changes time of the last bid change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the ::getTime() of this quote even precise up to a millisecond.
     *
     * @param bidTime time of the last bid change.
     */
    void setBidTime(std::int64_t bidTime) {
        data_.bidTime = bidTime;

        recomputeTimeMillisPart();
    }

    /**
     * Returns bid exchange code.
     *
     * @return bid exchange code.
     */
    char getBidExchangeCode() const;

    /**
     * Changes bid exchange code.
     *
     * @param bidExchangeCode bid exchange code.
     */
    void setBidExchangeCode(char bidExchangeCode);

    /**
     * Returns bid price.
     *
     * @return bid price.
     */
    double getBidPrice() const { return data_.bidPrice; }

    /**
     * Changes bid price.
     *
     * @param bidPrice bid price.
     */
    void setBidPrice(double bidPrice) { data_.bidPrice = bidPrice; }

    /**
     * Returns bid size.
     *
     * @return bid size
     */
    double getBidSize() const { return data_.bidSize; }

    /**
     * Changes bid size.
     *
     * @param bidSize bid size.
     */
    void setBidSize(double bidSize) { data_.bidSize = bidSize; }

    /**
     * Returns time of the last ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * <b>This time is always transmitted with seconds precision, so the result of this method is usually a multiple of
     * 1000.</b>
     *
     * @return time of the last ask change.
     */
    std::int64_t getAskTime() const { return data_.askTime; }

    /**
     * Changes time of the last ask change.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * You can set the actual millisecond-precision time here to publish event and the millisecond part
     * will make the ::getTime() of this quote even precise up to a millisecond.
     *
     * @param askTime time of the last ask change.
     */
    void setAskTime(std::int64_t askTime) {
        data_.askTime = askTime;

        recomputeTimeMillisPart();
    }

    /**
     * Returns ask exchange code.
     *
     * @return ask exchange code.
     */
    char getAskExchangeCode() const;

    /**
     * Changes ask exchange code.
     *
     * @param askExchangeCode ask exchange code.
     */
    void setAskExchangeCode(char bidExchangeCode);

    /**
     * Returns ask price.
     *
     * @return ask price.
     */
    double getAskPrice() const { return data_.askPrice; }

    /**
     * Changes ask price.
     *
     * @param askPrice ask price.
     */
    void setAskPrice(double askPrice) { data_.askPrice = askPrice; }

    /**
     * Returns ask size.
     *
     * @return ask size
     */
    double getAskSize() const { return data_.askSize; }

    /**
     * Changes ask size.
     *
     * @param askSize ask size.
     */
    void setAskSize(double askSize) { data_.askSize = askSize; }

    std::string toString() const {
        return detail::vformat(
            "Quote{{{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, bidTime={}, bidExchange={}, bidPrice={}, "
            "bidSize={}, askTime={}, askExchange={}, askPrice={}, askSize={}}}",
            MarketEvent::getEventSymbol(), detail::formatTimeStampWithMillis(MarketEvent::getEventTime()),
            detail::formatTimeStampWithMillis(getTime()), getTimeNanoPart(), getSequence(),
            detail::formatTimeStamp(getBidTime()), detail::string_util::encodeChar(data_.bidExchangeCode),
            getBidPrice(), getBidSize(), detail::formatTimeStamp(getAskTime()),
            detail::string_util::encodeChar(data_.askExchangeCode), getAskPrice(), getAskSize());
    }
};

} // namespace dxfcpp