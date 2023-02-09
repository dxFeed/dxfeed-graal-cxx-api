// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_subscription.h>

#include <memory>

namespace dxfcpp {

struct DXFeed;

template <typename Collection>
concept ElementIsEventTypeEnum =
    requires(Collection &&c) { std::is_same_v<std::decay_t<decltype(*std::begin(c))>, EventTypeEnum>; };

class DXFeedSubscription : public std::enable_shared_from_this<DXFeedSubscription>,
                           public detail::WithHandle<dxfg_subscription_t *> {
    friend struct DXFeed;

    mutable std::recursive_mutex mtx_{};

    explicit DXFeedSubscription(const EventTypeEnum &eventType) noexcept : mtx_{} {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription(eventType = {})", eventType.getName());
        }

        handle_ = detail::runIsolatedOrElse(
            [eventType](auto threadHandle) {
                return dxfg_DXFeedSubscription_new(threadHandle, eventType.getDxFeedGraalNativeApiEventClazz());
            },
            nullptr);
    }

    template <typename EventTypeIt> DXFeedSubscription(EventTypeIt begin, EventTypeIt end) noexcept : mtx_{} {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription(eventTypes = {})", detail::namesToString(begin, end));
        }

        dxfg_event_clazz_list_t l{0, nullptr};

        auto clazzListCleaner = detail::finally([&l] {
            if (l.size == 0 || l.elements == nullptr) {
                return;
            }

            for (auto i = l.size - 1; i >= 0; i--) {
                delete l.elements[i];
            }

            delete[] l.elements;
        });

        auto size = std::distance(begin, end);

        if (size <= 0) {
            return;
        }

        l.size = static_cast<std::int32_t>(size);
        l.elements = new (std::nothrow) dxfg_event_clazz_t *[size];

        if (!l.elements) {
            return;
        }

        std::size_t i = 0;

        for (auto it = begin; it != end; it++, i++) {
            l.elements[i] = new (std::nothrow) dxfg_event_clazz_t{it->getDxFeedGraalNativeApiEventClazz()};
        }

        handle_ = detail::runIsolatedOrElse(
            [&l](auto threadHandle) { return dxfg_DXFeedSubscription_new2(threadHandle, &l); }, nullptr);
    }

    DXFeedSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept
        : DXFeedSubscription(eventTypes.begin(), eventTypes.end()) {}

    template <typename EventTypesCollection>
    explicit DXFeedSubscription(EventTypesCollection &&eventTypes) noexcept
        requires requires { ElementIsEventTypeEnum<EventTypesCollection>; }
        : DXFeedSubscription(std::begin(std::forward<EventTypesCollection>(eventTypes)),
                             std::end(std::forward<EventTypesCollection>(eventTypes))) {}

  public:
    std::string toString() const {
        std::lock_guard lock(mtx_);

        return detail::vformat("DXFeedSubscription{{{}}}", detail::bit_cast<std::size_t>(handle_));
    }

    virtual ~DXFeedSubscription() {
        if constexpr (detail::isDebug) {
            detail::debug("{}::~DXFeedSubscription()", toString());
        }

        detail::tryCallWithLock(mtx_, [this] { releaseHandle(); });
    }

    static std::shared_ptr<DXFeedSubscription> create(const EventTypeEnum &eventType) noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription::create(eventType = {})", eventType.getName());
        }

        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventType));
    }

    template <typename EventTypeIt>
    static std::shared_ptr<DXFeedSubscription> create(EventTypeIt begin, EventTypeIt end) noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription::create(eventTypes = {})", detail::namesToString(begin, end));
        }

        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(begin, end));
    }

    static std::shared_ptr<DXFeedSubscription> create(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventTypes));
    }

    template <typename EventTypesCollection>
    static std::shared_ptr<DXFeedSubscription> create(EventTypesCollection &&eventTypes) noexcept
        requires requires { ElementIsEventTypeEnum<EventTypesCollection>; }
    {
        return std::shared_ptr<DXFeedSubscription>(
            new DXFeedSubscription(std::forward<EventTypesCollection>(eventTypes)));
    }

    void attach(std::shared_ptr<DXFeed> feed) noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("{}::attach(feed = {})", toString(), feed->toString());
        }

        feed->attachSubscription(shared_from_this());
    }
};

} // namespace dxfcpp