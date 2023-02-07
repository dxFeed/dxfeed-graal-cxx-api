// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_events.h>

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
    dxfg_event_clazz_t dxFeedGraalNativeApiEventClazz_;

    // A flag that indicates that the current enum element is characterizing the Lasting (TICKER) event.
    bool isLasting_;

    // A flag that indicates that the current enum element is characterizing the Indexed event.
    bool isIndexed_;

    // A flag that indicates that the current enum element is characterizing the TimeSeries (HISTORY) event.
    bool isTimeSeries_;

    // A flag that indicates that the current enum element is characterizing the OnlyIndexed (Indexed, but not
    // TimeSeries) event.
    bool isOnlyIndexed_;

    EventTypeEnum(dxfg_event_clazz_t dxFeedGraalNativeApiEventClazz, bool isLasting, bool isIndexed = false,
                  bool isTimeSeries = false)
        : dxFeedGraalNativeApiEventClazz_{dxFeedGraalNativeApiEventClazz}, isLasting_{isLasting},
          isIndexed_{isIndexed || isTimeSeries}, isTimeSeries_{isTimeSeries}, isOnlyIndexed_{isIndexed &&
                                                                                             !isTimeSeries} {}

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

    explicit EventTypeEnum() : EventTypeEnum{static_cast<dxfg_event_clazz_t>(-1), false} {}

    /**
     * @return The dxFeed Graal Native C-API event class id
     */
    dxfg_event_clazz_t getDxFeedGraalNativeApiEventClazz() const { return dxFeedGraalNativeApiEventClazz_; }

    bool operator==(const EventTypeEnum &eventTypeEnum) const {
        return dxFeedGraalNativeApiEventClazz_ == eventTypeEnum.dxFeedGraalNativeApiEventClazz_;
    }

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
    std::size_t operator()(const dxfcpp::EventTypeEnum &eventType) const noexcept {
        return eventType.getDxFeedGraalNativeApiEventClazz();
    }
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
    /**
     * Protected constructor for concrete implementation classes that initializes `eventSymbol` property.
     *
     * @param eventSymbol The event symbol.
     */
    explicit MarketEvent(std::string eventSymbol) : eventSymbol_{std::move(eventSymbol)} {}

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

/**
 * Represents an indexed collection of up-to-date information about some
 * condition or state of an external entity that updates in real-time. For example, ::Order represents an order to buy
 * or to sell some market instrument that is currently active on a market exchange and multiple
 * orders are active for each symbol at any given moment in time.
 * ::Candle represent snapshots of aggregate information about trading over a specific time period and there are
 * multiple periods available.
 * The ::Candle is also an example of ::TimeSeriesEvent that is a more specific event type.
 *
 * <p> Index for each event is available via @ref ::getIndex() "index" property.
 * Indexed events retain information about multiple events per symbol based on the event index
 * and are conflated based on the event index. Last indexed event for each symbol and index is always
 * delivered to event listeners on subscription, but intermediate (next-to-last) events for each
 * symbol+index pair are not queued anywhere, they are simply discarded as stale events.
 * More recent events represent an up-to-date information about some external entity.
 *
 * <h3>Event flags, transactions and snapshots</h3>
 *
 * Some indexed event sources provide a consistent view of a set of events for a given symbol. Their updates
 * may incorporate multiple changes that have to be processed at the same time.
 * The corresponding information is carried in @ref ::getEventFlags() "eventFlags" property.
 * Some indexed events, like ::Order, support multiple sources of information for the
 * same symbol. The event source is available via @ref ::getSource() "source" property.
 * The value of @ref ::getSource() "source" property is always @ref IndexedEventSource::DEFAULT "DEFAULT" for
 * time-series events and other singe-sourced events like ::Series, that do not support this feature.
 *
 * <p> The value of @ref ::getEventFlags() ::eventFlags property has several significant bits that are packed
 * into an integer in the following way:
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
 * However, event @ref ::getIndex() "index" is unique across the sources. This is achieved by allocating
 * an event-specific number of most significant bits of @ref ::getIndex() "index" for use as
 * a @ref ::getSource() "source" @ref IndexedEventSource::id() "id".
 *
 * <p> `TX` (bit 0) &mdash; ::TX_PENDING is an indicator of pending transactional update.
 * It can be retrieved from `eventFlags` with the following piece of code:
 *
 * <p>`bool txPending = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::@ref ::TX_PENDING
 * "TX_PENDING") != 0;`
 *
 * <p>When `txPending` is `true` it means, that an ongoing transaction update that spans multiple events is
 * in process. All events with `txPending` `true` shall be put into a separate <em>pending list</em>
 * for each source id and should be processed later when an event for this source id with `txPending` `false` comes.
 *
 * <p> `RE` (bit 1) &mdash; ::REMOVE_EVENT is used to indicate that that the event with the
 * corresponding index has to be removed.
 *
 * <p>`bool removeEvent = (event->{@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::@ref ::REMOVE_EVENT
 * "REMOVE_EVENT") != 0;`
 *
 * <p> `SB` (bit 2) &mdash; ::SNAPSHOT_BEGIN is used to indicate when the loading of a snapshot starts.
 *
 * <p>`bool snapshotBegin = (event->{@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::@ref ::SNAPSHOT_BEGIN
 * "SNAPSHOT_BEGIN") != 0;`
 *
 * <p> Snapshot load starts on new subscription and the first indexed event that arrives for each non-zero source id
 * on new subscription may have `snapshotBegin` set to `true`. It means, that an ongoing snapshot
 * consisting of multiple events is incoming. All events for this source id shall be put into a separate
 * <em>pending list</em> for each source id.
 *
 * <p> `SE` (bit 3) &mdash; ::SNAPSHOT_END or `SS` (bit 4) &mdash; ::SNAPSHOT_SNIP are used to indicate the end of a
 * snapshot.
 *
 * <p> `bool snapshotEnd = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::@ref ::SNAPSHOT_END
 * "SNAPSHOT_END") != 0;` <br>`bool snapshotSnip = (event->@ref ::getEventFlags() "getEventFlags"() &amp;
 * IndexedEvent::@ref ::SNAPSHOT_SNIP SNAPSHOT_SNIP) != 0;`
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
struct IndexedEvent {
    /// The alias to a type of shared pointer to the IndexedEvent object
    using Ptr = std::shared_ptr<IndexedEvent>;

    // The constants below must be synchronized with similar constants in EventFlag

    /**
     * Bit mask to get transaction pending indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * `bool txPending = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::TX_PENDING) != 0;`
     *
     * See "Event Flags" section.
     */
    static const std::uint32_t TX_PENDING = 0x01;

    /**
     * Bit mask to get removal indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * `bool removeEvent = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::REMOVE_EVENT != 0;`
     *
     * See "Event Flags" section.
     */
    static const std::uint32_t REMOVE_EVENT = 0x02;

    /**
     * Bit mask to get snapshot begin indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * `bool snapshotBegin = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::SNAPSHOT_BEGIN) != 0;`
     *
     * <p>See "Event Flags" section.
     */
    static const std::uint32_t SNAPSHOT_BEGIN = 0x04;

    /**
     * Bit mask to get snapshot end indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * `bool snapshotEnd = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::SNAPSHOT_END) != 0;`
     *
     * <p>See "Event Flags" section.
     */
    static const std::uint32_t SNAPSHOT_END = 0x08;

    /**
     * Bit mask to get snapshot snip indicator from the value of @ref ::getEventFlags() "eventFlags" property.
     *
     * `bool snapshotSnip = (event->@ref ::getEventFlags() "getEventFlags"() &amp; IndexedEvent::SNAPSHOT_SNIP) != 0;`
     *
     * See "Event Flags" section.
     */
    static const std::uint32_t SNAPSHOT_SNIP = 0x10;

    /**
     * Bit mask to set snapshot mode indicator into the value of @ref ::setEventFlags() "eventFlags" property.
     * This flag is intended for publishing only.
     *
     * See "Event Flags" section.
     */
    static const std::uint32_t SNAPSHOT_MODE = 0x40;

    /**
     * Returns the source of this event.
     *
     * @return The source of this event.
     */
    virtual const IndexedEventSource &getSource() const = 0;

    /**
     * Returns transactional event flags.
     * See "Event Flags" section.
     *
     * @return The transactional event flags.
     */
    virtual std::uint32_t getEventFlags() const = 0;

    /**
     * Changes transactional event flags.
     * See "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     */
    virtual void setEventFlags(std::uint32_t eventFlags) = 0;

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

} // namespace dxfcpp