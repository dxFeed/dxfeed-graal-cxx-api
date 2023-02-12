// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_subscription.h>

#include <memory>

#include "internal/Common.hpp"
#include "DXEvent.hpp"

#include <unordered_set>

namespace dxfcpp {

struct DXFeed;

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
        requires requires { detail::ElementTypeIs<EventTypesCollection, EventTypeEnum>; }
        : DXFeedSubscription(std::begin(std::forward<EventTypesCollection>(eventTypes)),
                             std::end(std::forward<EventTypesCollection>(eventTypes))) {}

    void closeImpl() noexcept {
        if (!handle_) {
            return;
        }

        detail::runIsolatedOrElse(
            [this](auto threadHandle) { return dxfg_DXFeedSubscription_close(threadHandle, handle_) == 0; }, false);
    }

    void clearImpl() noexcept {
        if (!handle_) {
            return;
        }

        detail::runIsolatedOrElse(
            [this](auto threadHandle) { return dxfg_DXFeedSubscription_clear(threadHandle, handle_) == 0; }, false);
    }

    bool isClosedImpl() noexcept {
        if (!handle_) {
            return false;
        }

        detail::runIsolatedOrElse(
            [this](auto threadHandle) { return dxfg_DXFeedSubscription_isClosed(threadHandle, handle_) != 0; }, false);
    }

  public:
    std::string toString() const {
        std::lock_guard lock(mtx_);

        return detail::vformat("DXFeedSubscription{{{}}}", detail::bit_cast<std::size_t>(handle_));
    }

    virtual ~DXFeedSubscription() {
        if constexpr (detail::isDebug) {
            detail::debug("{}::~DXFeedSubscription()", toString());
        }

        detail::tryCallWithLock(mtx_, [this] {
            closeImpl();
            releaseHandle();
        });
    }

    /**
     * Creates <i>detached</i> subscription for a single event type.
     *
     * @param eventType the event type.
     */
    static std::shared_ptr<DXFeedSubscription> create(const EventTypeEnum &eventType) noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription::create(eventType = {})", eventType.getName());
        }

        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventType));
    }

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * @tparam EventTypeIt The collection's iterator type
     * @param begin The beginning of the collection of event types.
     * @param end The end of event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    template <typename EventTypeIt>
    static std::shared_ptr<DXFeedSubscription> create(EventTypeIt begin, EventTypeIt end) noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeedSubscription::create(eventTypes = {})", detail::namesToString(begin, end));
        }

        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(begin, end));
    }

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * @param eventTypes The event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    static std::shared_ptr<DXFeedSubscription> create(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
        return std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventTypes));
    }

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * @tparam EventTypesCollection The type of the collection of event types
     * @param eventTypes The event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    template <typename EventTypesCollection>
    static std::shared_ptr<DXFeedSubscription> create(EventTypesCollection &&eventTypes) noexcept
        requires requires { detail::ElementTypeIs<EventTypesCollection, EventTypeEnum>; }
    {
        return std::shared_ptr<DXFeedSubscription>(
            new DXFeedSubscription(std::forward<EventTypesCollection>(eventTypes)));
    }

    /**
     * Attaches subscription to the specified feed.
     *
     * @param feed The feed to attach to.
     */
    void attach(std::shared_ptr<DXFeed> feed) noexcept;

    /**
     * Detaches subscription from the specified feed.
     *
     * @param feed The feed to detach from.
     */
    void detach(std::shared_ptr<DXFeed> feed) noexcept;

    /**
     * Closes this subscription and makes it <i>permanently detached</i>.
     * This method notifies all installed instances of subscription change listeners by invoking `subscriptionClosed`
     * while holding the lock for this subscription. This method clears lists of all installed
     * event listeners and subscription change listeners and makes sure that no more listeners
     * can be added.
     */
    void close() noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("{}::close()", toString());
        }

        std::lock_guard lock(mtx_);

        closeImpl();
    }

    template <typename EventListener>
    std::size_t addEventListener(EventListener&& listener) noexcept;

    void removeEventListener(std::size_t listenerId) noexcept;

    auto onEvent() noexcept;

    template <typename Symbol>
    void addSymbol(Symbol&& symbol) noexcept;

    template <typename SymbolsCollection>
    void addSymbols(SymbolsCollection&& collection) noexcept;

    template <typename Symbol>
    void removeSymbol(Symbol&& symbol) noexcept;

    template <typename SymbolsCollection>
    void removeSymbols(SymbolsCollection&& collection) noexcept;

    /**
     * Clears the set of subscribed symbols.
     */
    void clear() noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("{}::clear()", toString());
        }

        std::lock_guard lock(mtx_);

        clearImpl();
    }

    /**
     * Returns `true` if this subscription is closed.
     *
     * @return `true` if this subscription is closed.
     *
     * @see ::close
     */
    bool isClosed() noexcept {
        if constexpr (detail::isDebug) {
            detail::debug("{}::isClosed()", toString());
        }

        std::lock_guard lock(mtx_);

        return isClosedImpl();
    }

    /**
     * Returns a set of subscribed event types.
     *
     * @return A set of subscribed event types.
     */
    std::unordered_set<EventTypeEnum> getEventTypes() noexcept;

    /**
     * Returns `true` if this subscription contains the corresponding event type.
     *
     * @return `true` if this subscription contains the corresponding event type.
     *
     * @see ::getEventTypes()
     */
    bool containsEventType(const EventTypeEnum& eventType) noexcept;

    /**
     * Returns a set of subscribed symbols. The resulting set maybe either a snapshot of the set of
     * the subscribed symbols at the time of invocation or a weakly consistent view of the set.
     *
     * @return A set of subscribed symbols.
     */
    auto getSymbols() noexcept;

    template <typename Symbol>
    void setSymbol(Symbol&& symbol) noexcept;

    template <typename SymbolsCollection>
    void setSymbols(SymbolsCollection&& collection) noexcept;

    /**
     * Returns a set of decorated symbols (depending on the actual implementation of subscription).
     *
     * The resulting set maybe either a snapshot of the set of
     * the subscribed symbols at the time of invocation or a weakly consistent view of the set.
     */
    auto getDecoratedSymbols() noexcept;

    auto getExecutor() noexcept;

    template <typename Executor>
    void setExecutor(Executor&& executor) noexcept;

    template <typename ChangeListener>
    std::size_t addChangeListener(ChangeListener&& changeListener) noexcept;

    void removeChangeListener(std::size_t changeListenerId) noexcept;

    auto onSymbolsAdded() noexcept;

    auto onSymbolsRemoved() noexcept;

    auto onSubscriptionClosed() noexcept;
};

} // namespace dxfcpp