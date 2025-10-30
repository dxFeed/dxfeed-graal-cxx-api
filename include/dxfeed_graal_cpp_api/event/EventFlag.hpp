// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

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
 * bool txPending = (event->getEventFlags() & IndexedEvent::TX_PENDING) != 0;
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
 * bool removeEvent = (event->getEventFlags() & IndexedEvent::REMOVE_EVENT) != 0;
 * ```
 *
 * <p> `SB` (bit 2) &mdash; ::SNAPSHOT_BEGIN is used to indicate when the loading of a snapshot starts.
 *
 * ```cpp
 * bool snapshotBegin = (event->getEventFlags() & IndexedEvent::SNAPSHOT_BEGIN) != 0;
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
 * bool snapshotEnd = (event->getEventFlags() & IndexedEvent::SNAPSHOT_END) != 0;
 * bool snapshotSnip = (event->getEventFlags() & IndexedEvent::SNAPSHOT_SNIP) != 0;
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
class DXFCPP_EXPORT EventFlag final {
    std::uint32_t flag_;
    std::string name_;

    EventFlag(std::uint32_t flag, std::string name) noexcept : flag_{flag}, name_{std::move(name)} {
    }

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
    explicit EventFlag() noexcept : flag_{unsigned(-1)}, name_{"INVALID"} {
    }

    /**
     * @return The event flag's value
     */
    std::uint32_t getFlag() const noexcept {
        return flag_;
    }

    /**
     * Determines if the given flag is in the mask.
     *
     * @param eventFlagsMask The event flags mask
     *
     * @return `true` the given flag is in the mask.
     */
    bool in(std::uint32_t eventFlagsMask) const noexcept {
        return (eventFlagsMask & flag_) != 0;
    }

    /**
     * Determines if the given flag is in the mask.
     *
     * @tparam EventFlagsMask An event flags mask type that satisfies the condition: there is a `getMask` method that
     * returns std::uint32_t
     * @param eventFlagsMask The event flags mask.
     * @return `true` the given flag is in the mask.
     */
    template <typename EventFlagsMask>
    bool in(const EventFlagsMask &eventFlagsMask) const noexcept
#if __cpp_concepts
        requires requires {
            { eventFlagsMask.getMask() } -> std::same_as<std::uint32_t>;
        }
#endif
    {
        return in(eventFlagsMask.getMask());
    }

    ///
    const std::string &getName() const & noexcept {
        return name_;
    }

    ///
    const std::string &toString() const & noexcept {
        return name_;
    }

    /**
     * Performs a bit `or` operation with two event flags
     *
     * @param eventFlag1 The first event flag
     * @param eventFlag2 The second event flag (std::int32_t)
     * @return The result (std::int32_t)
     */
    friend std::int32_t operator|(const EventFlag &eventFlag1, std::int32_t eventFlag2) noexcept {
        return static_cast<std::int32_t>(dxfcpp::orOp(eventFlag1.getFlag(), eventFlag2));
    }

    /**
     * Performs a bit `or` operation with two event flags
     *
     * @param eventFlag1 The first event flag (std::int32_t)
     * @param eventFlag2 The second event flag
     * @return The result (std::int32_t)
     */
    friend std::int32_t operator|(std::int32_t eventFlag1, const EventFlag &eventFlag2) noexcept {
        return dxfcpp::orOp(eventFlag1, eventFlag2.getFlag());
    }

    /**
     * Performs a bit `and` operation with two event flags
     *
     * @param eventFlag1 The first event flag
     * @param eventFlag2 The second event flag (std::int32_t)
     * @return The result (std::int32_t)
     */
    friend std::int32_t operator&(const EventFlag &eventFlag1, std::int32_t eventFlag2) noexcept {
        return static_cast<std::int32_t>(dxfcpp::andOp(eventFlag1.getFlag(), eventFlag2));
    }

    /**
     * Performs a bit `and` operation with two event flags
     *
     * @param eventFlag1 The first event flag (std::int32_t)
     * @param eventFlag2 The second event flag
     * @return The result (std::int32_t)
     */
    friend std::int32_t operator&(std::int32_t eventFlag1, const EventFlag &eventFlag2) noexcept {
        return dxfcpp::andOp(eventFlag1, eventFlag2.getFlag());
    }

    /**
     * Performs a bit `or` operation with two event flags
     *
     * @param eventFlag1 The first event flag
     * @param eventFlag2 The second event flag (std::uint32_t)
     * @return The result (std::uint32_t)
     */
    friend std::uint32_t operator|(const EventFlag &eventFlag1, std::uint32_t eventFlag2) noexcept {
        return eventFlag1.getFlag() | eventFlag2;
    }

    /**
     * Performs a bit `or` operation with two event flags
     *
     * @param eventFlag1 The first event flag (std::uint32_t)
     * @param eventFlag2 The second event flag
     * @return The result (std::uint32_t)
     */
    friend std::uint32_t operator|(std::uint32_t eventFlag1, const EventFlag &eventFlag2) noexcept {
        return dxfcpp::orOp(eventFlag1, eventFlag2.getFlag());
    }

    /**
     * Performs a bit `and` operation with two event flags
     *
     * @param eventFlag1 The first event flag
     * @param eventFlag2 The second event flag (std::uint32_t)
     * @return The result (std::uint32_t)
     */
    friend std::uint32_t operator&(const EventFlag &eventFlag1, std::uint32_t eventFlag2) noexcept {
        return static_cast<std::int32_t>(dxfcpp::andOp(eventFlag1.getFlag(), eventFlag2));
    }

    /**
     * Performs a bit `and` operation with two event flags
     *
     * @param eventFlag1 The first event flag (std::uint32_t)
     * @param eventFlag2 The second event flag
     * @return The result (std::uint32_t)
     */
    friend std::uint32_t operator&(std::uint32_t eventFlag1, const EventFlag &eventFlag2) noexcept {
        return dxfcpp::andOp(eventFlag1, eventFlag2.getFlag());
    }

    /**
     * Determines if the given event marks the beginning of a snapshot.
     *
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate the beginning of a snapshot, `false` otherwise.
     */
    template <typename Event> static bool isSnapshotBegin(const std::shared_ptr<Event> &event) {
        return (event->getEventFlags() & SNAPSHOT_BEGIN) != 0;
    }

    /**
     * Determines if the given event marks the end of a snapshot.
     *
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate the end of a snapshot, `false` otherwise.
     */
    template <typename Event> static bool isSnapshotEnd(const std::shared_ptr<Event> &event) {
        return (event->getEventFlags() & SNAPSHOT_END) != 0;
    }

    /**
     * Determines if the given event is marked as a snapshot snip.
     *
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate a snapshot snip, `false` otherwise.
     */
    template <typename Event> static bool isSnapshotSnip(const std::shared_ptr<Event> &event) {
        return (event->getEventFlags() & SNAPSHOT_SNIP) != 0;
    }

    /**
     * Determines if the given event marks the end of a snapshot or a snapshot snip.
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate the end or snip of a snapshot, `false` otherwise.
     */
    template <typename Event> static bool isSnapshotEndOrSnip(const std::shared_ptr<Event> &event) {
        return isSnapshotEnd(event) || isSnapshotSnip(event);
    }

    /**
     * Determines if the given event is in a pending state.
     *
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate a pending transaction, `false` otherwise.
     */
    template <typename Event> static bool isPending(const std::shared_ptr<Event> &event) {
        return (event->getEventFlags() & TX_PENDING) != 0;
    }

    /**
     * Determines if the given event is marked for removal.
     *
     * @tparam Event The event's type.
     * @param event The event to be checked.
     * @return `true` if the event flags indicate a remove action, `false` otherwise.
     */
    template <typename Event> static bool isRemove(const std::shared_ptr<Event> &event) {
        return (event->getEventFlags() & REMOVE_EVENT) != 0;
    }
};

DXFCPP_END_NAMESPACE

///
template <> struct std::hash<dxfcpp::EventFlag> {
    std::size_t operator()(const dxfcpp::EventFlag &eventFlag) const noexcept {
        return eventFlag.getFlag();
    }
};

DXFCPP_BEGIN_NAMESPACE

/// The event flags' mask (a set of bit flags)
class EventFlagsMask final {
    std::uint32_t mask_;

    public:
    /**
     * Creates an empty event flags mask
     */
    explicit EventFlagsMask() noexcept : mask_{0u} {
    }

    /**
     * Create event flags mask by integer value
     *
     * @tparam MaskType The type of integer mask
     * @param mask The integer mask value
     */
    template <Integral MaskType>
    explicit EventFlagsMask(MaskType mask) noexcept : mask_{static_cast<std::uint32_t>(mask)} {
    }

    /**
     * Creates event flags mask by iterators of container with flags
     *
     * @tparam EventFlagIt The iterator type
     * @param begin The start position
     * @param end The end position
     */
    template <typename EventFlagIt> EventFlagsMask(EventFlagIt begin, EventFlagIt end) noexcept {
        mask_ = std::accumulate(begin, end, 0u, [](unsigned mask, const EventFlag &flag) {
            return mask | flag.getFlag();
        });
    }

    /**
     * Creates event flags mask by initializer list with flags
     *
     * @param eventFlags The list with flags
     */
    EventFlagsMask(std::initializer_list<EventFlag> eventFlags) noexcept
        : EventFlagsMask(eventFlags.begin(), eventFlags.end()) {
    }

    /**
     * Returns an integer representation of an event mask
     *
     * @return an integer representation
     */
    constexpr std::uint32_t getMask() const noexcept {
        return mask_;
    }

    /**
     * @param flag The flag to check.
     * @return `true` if this mask contains the flag.
     */
    bool contains(const EventFlag &flag) const noexcept {
        return (mask_ & flag.getFlag()) != 0;
    }

    /**
     * Performs a bit `or` operation with an event flags' mask and an event flag
     *
     * @param eventFlagsMask The event flags' mask
     * @param eventFlag The event flag
     * @return The result (event flags' mask)
     */
    friend EventFlagsMask operator|(const EventFlagsMask &eventFlagsMask, const EventFlag &eventFlag) noexcept {
        return EventFlagsMask{eventFlagsMask.mask_ | eventFlag.getFlag()};
    }

    /**
     * Performs a bit `and` operation with an event flags' mask and an event flag
     *
     * @param eventFlagsMask The event flags' mask
     * @param eventFlag The event flag
     * @return The result (event flags' mask)
     */
    friend EventFlagsMask operator&(const EventFlagsMask &eventFlagsMask, const EventFlag &eventFlag) noexcept {
        return EventFlagsMask{eventFlagsMask.mask_ & eventFlag.getFlag()};
    }

    ///
    std::string toString() const {
        bool addOrSign = false;
        std::ostringstream result{};

        for (const auto &flag :
             {EventFlag::TX_PENDING, EventFlag::REMOVE_EVENT, EventFlag::SNAPSHOT_BEGIN, EventFlag::SNAPSHOT_END,
              EventFlag::SNAPSHOT_SNIP, EventFlag::SNAPSHOT_MODE, EventFlag::REMOVE_SYMBOL}) {
            if (flag.in(mask_)) {
                if (addOrSign) {
                    result << "|";
                } else {
                    addOrSign = true;
                }

                result << flag.toString();
            }
        }

        result << "(0x" << std::hex << mask_ << ")";

        return result.str();
    }
};

DXFCPP_END_NAMESPACE

/**
 * Performs a bit `or` operation with two event flags
 *
 * @param eventFlag1 The first event flag
 * @param eventFlag2 The second event flag
 * @return The result (event flags' mask )
 */
inline dxfcpp::EventFlagsMask operator|(const dxfcpp::EventFlag &eventFlag1,
                                        const dxfcpp::EventFlag &eventFlag2) noexcept {
    return dxfcpp::EventFlagsMask{eventFlag1.getFlag() | eventFlag2.getFlag()};
}

DXFCXX_DISABLE_MSC_WARNINGS_POP()