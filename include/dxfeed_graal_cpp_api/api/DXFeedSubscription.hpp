// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

#include "../event/DXEvent.hpp"
#include "../internal/Common.hpp"
#include "../symbols/StringSymbol.hpp"
#include "../symbols/SymbolWrapper.hpp"
#include "dxfeed_graal_cpp_api/api/osub/WildcardSymbol.hpp"

#if __cpp_lib_parallel_algorithm
#    include <execution>
#endif

#include <unordered_set>

namespace dxfcpp {

struct DXFeed;

class DXFeedSubscription : public SharedEntity {
    friend struct DXFeed;

    mutable std::recursive_mutex mtx_{};
    JavaObjectHandler<DXFeedSubscription> handler_;
    JavaObjectHandler<DXFeedEventListener> eventListenerHandler_;
    Handler<void(const std::vector<std::shared_ptr<EventType>> &)> onEvent_{1};

    explicit DXFeedSubscription(const EventTypeEnum &eventType) noexcept;

    static JavaObjectHandler<DXFeedSubscription>
    createSubscriptionHandlerFromEventClassList(const std::unique_ptr<EventClassList> &list) noexcept;

    void setEventListenerHandler(Id<DXFeedSubscription> id) noexcept;

    template <typename EventTypeIt>
    DXFeedSubscription(EventTypeIt begin, EventTypeIt end) noexcept
        : mtx_{}, handler_{}, eventListenerHandler_{}, onEvent_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeedSubscription(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto list = EventClassList::create(begin, end);

        if (!list) {
            return;
        }

        handler_ = createSubscriptionHandlerFromEventClassList(list);
    }

    DXFeedSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept
        : DXFeedSubscription(eventTypes.begin(), eventTypes.end()) {}

    template <typename EventTypesCollection>
    explicit DXFeedSubscription(EventTypesCollection &&eventTypes) noexcept
#if __cpp_concepts
        requires ElementTypeIs<EventTypesCollection, EventTypeEnum>
#endif
        : DXFeedSubscription(std::begin(std::forward<EventTypesCollection>(eventTypes)),
                             std::end(std::forward<EventTypesCollection>(eventTypes))) {
    }

    void closeImpl() const noexcept;

    void clearImpl() const noexcept;

    bool isClosedImpl() const noexcept;

    void addSymbolImpl(void *graalSymbol) const noexcept;

    void removeSymbolImpl(void *graalSymbol) const noexcept;

  public:
    std::string toString() const noexcept override;

    ~DXFeedSubscription() override {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeedSubscription{" + handler_.toString() + "}::~DXFeedSubscription()");
        }

        tryCallWithLock(mtx_, [this] { closeImpl(); });
    }

    /**
     * Creates <i>detached</i> subscription for a single event type.
     *
     * @param eventType the event type.
     */
    static std::shared_ptr<DXFeedSubscription> create(const EventTypeEnum &eventType) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeedSubscription::create(eventType = " + eventType.getName() + ")");
        }

        auto sub = std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventType));
        auto id = ApiContext::getInstance()->getDxFeedSubscriptionManager()->registerEntity(sub);

        sub->setEventListenerHandler(id);

        return sub;
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
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeedSubscription::create(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto sub = std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(begin, end));
        auto id = ApiContext::getInstance()->getDxFeedSubscriptionManager()->registerEntity(sub);

        sub->setEventListenerHandler(id);

        return sub;
    }

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * @param eventTypes The event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    static std::shared_ptr<DXFeedSubscription> create(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
        auto sub = std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(eventTypes));
        auto id = ApiContext::getInstance()->getDxFeedSubscriptionManager()->registerEntity(sub);

        sub->setEventListenerHandler(id);

        return sub;
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
#if __cpp_concepts
        requires ElementTypeIs<EventTypesCollection, EventTypeEnum>
#endif
    {
        auto sub =
            std::shared_ptr<DXFeedSubscription>(new DXFeedSubscription(std::forward<EventTypesCollection>(eventTypes)));
        auto id = ApiContext::getInstance()->getDxFeedSubscriptionManager()->registerEntity(sub);

        sub->setEventListenerHandler(id);

        return sub;
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
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::close()");
        }

        std::lock_guard lock(mtx_);

        closeImpl();
    }

    /**
     * Adds listener for events.
     * Event lister can be added only when subscription is not producing any events.
     * The subscription must be either empty
     * (its set of @ref ::getSymbols() "symbols" is empty or not @ref ::attach() "attached" to any feed
     * (its set of change listeners is empty).
     *
     * This method does nothing if this subscription is closed.
     *
     * @tparam EventListener The listener type. Listener can be callable with signature: `void(const
     * std::vector<std::shared_ptr<EventType>&)`
     * @param listener The event listener
     * @return The listener id
     */
    template <typename EventListener>
    std::size_t addEventListener(EventListener &&listener) noexcept
#if __cpp_concepts
        requires requires {
            { listener(std::vector<std::shared_ptr<EventType>>{}) } -> std::same_as<void>;
        }
#endif
    {
        return onEvent_ += listener;
    }

    /**
     * Removes listener for events.
     *
     * @param listenerId The listener id
     */
    void removeEventListener(std::size_t listenerId) noexcept { onEvent_ -= listenerId; }

    /**
     * Returns a reference to an incoming events' handler (delegate), to which listeners can be added and removed.
     *
     * @return The incoming events' handler (delegate)
     */
    const auto &onEvent() noexcept { return onEvent_; }

    void addSymbols(const SymbolWrapper &symbolWrapper) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::addSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
        }

        addSymbolImpl(symbolWrapper.toGraal());
    }

    void removeSymbols(const SymbolWrapper &symbolWrapper) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::removeSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
        }

        removeSymbolImpl(symbolWrapper.toGraal());
    }

    template <typename SymbolIt> void addSymbols(SymbolIt begin, SymbolIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::addSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        std::for_each(
#if __cpp_lib_parallel_algorithm
            std::execution::par,
#endif
            begin, end, [this](const auto &wrapper) { addSymbolImpl(wrapper.toGraal()); });
    }

    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void addSymbols(const SymbolsCollection &collection) noexcept {
        addSymbols(std::begin(collection), std::end(collection));
    }

    void addSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        addSymbols(collection.begin(), collection.end());
    }

    template <typename SymbolIt> void removeSymbols(SymbolIt begin, SymbolIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::removeSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        std::for_each(
#if __cpp_lib_parallel_algorithm
            std::execution::par,
#endif
            begin, end,
                      [this](const auto &wrapper) { removeSymbolImpl(wrapper.toGraal()); });
    }

    void removeSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        removeSymbols(collection.begin(), collection.end());
    }

    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void removeSymbols(SymbolsCollection &&collection) noexcept {
        removeSymbols(std::begin(collection), std::end(collection));
    }

    template <typename SymbolIt> void setSymbols(SymbolIt begin, SymbolIt end) noexcept {
        //TODO: implement using the native implementation
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::setSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        clearImpl();
        addSymbols(begin, end);
    }

    void setSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        setSymbols(collection.begin(), collection.end());
    }

    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void setSymbols(SymbolsCollection &&collection) noexcept {
        setSymbols(std::begin(collection), std::end(collection));
    }

    /**
     * Clears the set of subscribed symbols.
     */
    void clear() noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::clear()");
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
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::isClosed()");
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
    bool containsEventType(const EventTypeEnum &eventType) noexcept;

    /**
     * Returns a set of subscribed symbols. The resulting set maybe either a snapshot of the set of
     * the subscribed symbols at the time of invocation or a weakly consistent view of the set.
     *
     * @return A set of subscribed symbols.
     */
    auto getSymbols() noexcept;

    template <typename Symbol> void setSymbol(Symbol &&symbol) noexcept;

    template <typename SymbolsCollection> void setSymbols(SymbolsCollection &&collection) noexcept;

    /**
     * Returns a set of decorated symbols (depending on the actual implementation of subscription).
     *
     * The resulting set maybe either a snapshot of the set of
     * the subscribed symbols at the time of invocation or a weakly consistent view of the set.
     */
    auto getDecoratedSymbols() noexcept;

    auto getExecutor() noexcept;

    template <typename Executor> void setExecutor(Executor &&executor) noexcept;

    template <typename ChangeListener> std::size_t addChangeListener(ChangeListener &&changeListener) noexcept;

    void removeChangeListener(std::size_t changeListenerId) noexcept;

    auto onSymbolsAdded() noexcept;

    auto onSymbolsRemoved() noexcept;

    auto onSubscriptionClosed() noexcept;
};

} // namespace dxfcpp