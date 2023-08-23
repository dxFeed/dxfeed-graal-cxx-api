// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandler.hpp"

#include "DXFeedSubscription.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

namespace dxfcpp {

struct DXEndpoint;
class EventTypeEnum;

/**
 * Main entry class for dxFeed API (<b>read it first</b>).
 */
struct DXFCPP_EXPORT DXFeed : SharedEntity {
    friend struct DXEndpoint;

  private:
    JavaObjectHandler<DXFeed> handler_;

    std::unordered_set<std::shared_ptr<DXFeedSubscription>> subscriptions_{};

    static std::shared_ptr<DXFeed> create(void *feedHandle) noexcept;

  protected:
    DXFeed() noexcept : handler_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeed()");
        }
    }

  public:
    ~DXFeed() noexcept override {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeed{" + handler_.toString() + "}::~DXFeed()");
        }
    }

    /**
     * Returns a default application-wide singleton instance of feed. Most applications use only a single
     * data-source and should rely on this method to get one. This is a shortcut to
     * @ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::getInstance() "getInstance()"->@ref DXEndpoint::getFeed()
     * "getFeed()".
     *
     * @return The DXFeed instance
     */
    static std::shared_ptr<DXFeed> getInstance() noexcept;

    /**
     * Attaches the given subscription to this feed. This method does nothing if the
     * corresponding subscription is already attached to this feed.
     *
     * <p> This feed publishes data to the attached subscription.
     * Application can attach event listener via DXFeedSubscription::addEventListener to get notified about data changes
     * and can change its data subscription via DXFeedSubscription methods.
     *
     * <h3>Implementation notes</h3>
     *
     * This method adds a non-serializable ObservableSubscriptionChangeListener for the given subscription
     * via DXFeedSubscription::addChangeListener method.
     *
     * @param subscription The subscription.
     * @see DXFeedSubscription
     */
    void attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    /**
     * Detaches the given subscription from this feed. This method does nothing if the
     * corresponding subscription is not attached to this feed.
     *
     * <h3>Implementation notes</h3>
     *
     * This method removes ObservableSubscriptionChangeListener from the given subscription
     * via DXFeedSubscription::removeChangeListener method.
     *
     * @param subscription The subscription.
     * @see DXFeedSubscription
     */
    void detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    /**
     * Detaches the given subscription from this feed and clears data delivered to this subscription
     * by publishing empty events. This method does nothing if the
     * corresponding subscription is not attached to this feed.
     *
     * @param subscription The subscription.
     * @see ::detachSubscription()
     */
    void detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    /**
     * Creates new subscription for a single event type that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
     *
     * @param eventType The type of event
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(const EventTypeEnum &eventType) noexcept;

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
     *
     * @tparam EventTypeIt The iterator type of the collection of event types
     * @param begin The start iterator
     * @param end The end iterator
     * @return The new subscription
     */
    template <typename EventTypeIt>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypeIt begin, EventTypeIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("{}::createSubscription(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto sub = DXFeedSubscription::create(begin, end);

        attachSubscription(sub);

        return sub;
    }

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
     *
     * @param eventTypes The initializer list of event types
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept;

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
     *
     * @tparam EventTypesCollection The class of the collection of event types
     * @param eventTypes The collection of event types
     * @return The new subscription
     */
    template <typename EventTypesCollection>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypesCollection &&eventTypes) noexcept
    {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::createSubscription(eventTypes = " +
                            namesToString(std::begin(eventTypes), std::end(eventTypes)) + ")");
        }

        auto sub = DXFeedSubscription::create(eventTypes);

        attachSubscription(sub);

        return sub;
    }

    std::string toString() const noexcept override;
};

} // namespace dxfcpp