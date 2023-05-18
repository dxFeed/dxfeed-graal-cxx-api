// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"

#include "../event/DXEvent.hpp"
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
struct DXFeed : std::enable_shared_from_this<DXFeed> {
    friend struct DXEndpoint;

  private:
    handler_utils::JavaObjectHandler<DXFeed> handler_;

    std::unordered_set<std::shared_ptr<DXFeedSubscription>> subscriptions_{};

    static std::shared_ptr<DXFeed> create(void *feedHandle) noexcept;

  protected:
    DXFeed() noexcept : handler_{} {
        if constexpr (isDebug) {
            debug("DXFeed()");
        }
    }

  public:
    virtual ~DXFeed() noexcept {
        if constexpr (isDebug) {
            debug("{}::~DXFeed()", toString());
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

    void attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    void detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    void detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept;

    std::shared_ptr<DXFeedSubscription> createSubscription(const EventTypeEnum &eventType) noexcept;

    template <typename EventTypeIt>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypeIt begin, EventTypeIt end) noexcept {
        if constexpr (isDebug) {
            debug("{}::createSubscription(eventTypes = {})", namesToString(begin, end));
        }

        auto sub = DXFeedSubscription::create(begin, end);

        attachSubscription(sub);

        return sub;
    }

    std::shared_ptr<DXFeedSubscription> createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept;

    template <typename EventTypesCollection>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypesCollection &&eventTypes) noexcept
        requires requires { ElementTypeIs<EventTypesCollection, EventTypeEnum>; }
    {
        if constexpr (isDebug) {
            debug("{}::createSubscription(eventTypes = {})", toString(),
                  namesToString(std::begin(eventTypes), std::end(eventTypes)));
        }

        auto sub = DXFeedSubscription::create(eventTypes);

        attachSubscription(sub);

        return sub;
    }

    std::string toString() const {
        return fmt::format("DXFeed{{{}}}", handler_.toString());
    }
};

} // namespace dxfcpp