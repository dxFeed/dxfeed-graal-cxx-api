// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../event/DXEvent.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/JavaObjectHandler.hpp"
#include "../symbols/StringSymbol.hpp"
#include "../symbols/SymbolWrapper.hpp"
#include "dxfeed_graal_cpp_api/api/osub/WildcardSymbol.hpp"

#if __cpp_lib_parallel_algorithm
#    include <execution>
#endif

#include <memory>
#include <unordered_set>

namespace dxfcpp {

struct DXFeed;

/**
 * Subscription for a set of symbols and event types.
 */
class DXFCPP_EXPORT DXFeedSubscription : public SharedEntity {
    friend struct DXFeed;

    mutable std::recursive_mutex mtx_{};

    std::unordered_set<EventTypeEnum> eventTypes_;
    JavaObjectHandler<DXFeedSubscription> handler_;
    JavaObjectHandler<DXFeedEventListener> eventListenerHandler_;
    Handler<void(const std::vector<std::shared_ptr<EventType>> &)> onEvent_{1};

    explicit DXFeedSubscription(const EventTypeEnum &eventType) noexcept;

    static JavaObjectHandler<DXFeedSubscription>
    createSubscriptionHandlerFromEventClassList(const std::unique_ptr<EventClassList> &list) noexcept;

    void setEventListenerHandler(Id<DXFeedSubscription> id) noexcept;

    template <typename EventTypeIt>
#if __cpp_concepts
        requires requires(EventTypeIt iter) {
            { *iter } -> std::convertible_to<EventTypeEnum>;
        }
#endif
    DXFeedSubscription(EventTypeIt begin, EventTypeIt end) noexcept
        : mtx_{}, eventTypes_(begin, end), handler_{}, eventListenerHandler_{}, onEvent_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeedSubscription(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto list = EventClassList::create(eventTypes_.begin(), eventTypes_.end());

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
    ///
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
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription(Quote::Type);
     * ```
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
     * Example:
     * ```cpp
     * auto eventTypes = {dxfcpp::Quote::Type, dxfcpp::TimeAndSale::Type};
     * auto sub = dxfcpp::DXFeedSubscription::create(eventTypes.begin(), eventTypes.end());
     * ```
     *
     * @tparam EventTypeIt The collection's iterator type
     * @param begin The beginning of the collection of event types.
     * @param end The end of event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    template <typename EventTypeIt>
#if __cpp_concepts
        requires requires(EventTypeIt iter) {
            { *iter } -> std::convertible_to<EventTypeEnum>;
        }
#endif
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
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription::create({dxfcpp::Quote::Type, dxfcpp::TimeAndSale::Type});
     * ```
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
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription::create(std::unordered_set{dxfcpp::Quote::Type,
     * dxfcpp::TimeAndSale::Type});
     * ```
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
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::Type, dxfcpp::TimeAndSale::Type});
     *
     * sub->addEventListener([](auto &&events) {
     *     for (const auto &e : events) {
     *         if (auto quote = e->template sharedAs<dxfcpp::Quote>(); quote) {
     *             std::cout << "Q   : " + quote->toString() << std::endl;
     *         } else if (auto tns = e->template sharedAs<dxfcpp::TimeAndSale>(); tns) {
     *             std::cout << "TnS : " + tns->toString() << std::endl;
     *         }
     *     }
     * });
     *
     * sub->addSymbols({"$TOP10L/Q", "$SP500#45", "$TICK", "SPX"});
     * ```
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
     * Adds typed listener for events.
     * Event lister can be added only when subscription is not producing any events.
     * The subscription must be either empty
     * (its set of @ref ::getSymbols() "symbols" is empty or not @ref ::attach() "attached" to any feed
     * (its set of change listeners is empty).
     *
     * This method does nothing if this subscription is closed.
     *
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::Type, dxfcpp::TimeAndSale::Type});
     *
     * sub->addEventListener(std::function([](const std::vector<std::shared_ptr<dxfcpp::Quotes>> &quotes) -> void {
     *     for (const auto &q : quotes) {
     *         std::cout << "Q    : " + q->toString() << std::endl;
     *     }
     * }));
     *
     * sub->addEventListener<dxfcpp::TimeAndSale>([](const auto &timeAndSales) -> void {
     *     for (const auto &tns : timeAndSales) {
     *         std::cout << "TnS : " + tns->toString() << std::endl;
     *     }
     * });
     *
     * sub->addSymbols({"$TOP10L/Q", "AAPL", "$TICK", "SPX"});
     * ```
     *
     * @tparam EventT The event type (EventType's child with field Type, convertible to EventTypeEnum
     * @param listener The listener. Listener can be callable with signature: `void(const
     * std::vector<std::shared_ptr<EventT>&)`
     * @return The listener id
     */
    template <typename EventT>
    std::size_t addEventListener(std::function<void(const std::vector<std::shared_ptr<EventT>> &)> &&listener) noexcept
#if __cpp_concepts
        requires std::is_base_of_v<EventType, EventT> && requires {
            { EventT::TYPE } -> std::convertible_to<EventTypeEnum>;
        }
#endif
    {
        if (!containsEventType(EventT::TYPE)) {
            return onEvent_ += [](auto) {};
        }

        return onEvent_ += [l = listener](auto &&events) {
            std::vector<std::shared_ptr<EventT>> filteredEvents{};

            filteredEvents.reserve(events.size());

            for (const auto &e : events) {
                if (auto expected = e->template sharedAs<EventT>(); expected) {
                    filteredEvents.emplace_back(expected);
                }
            }

            l(filteredEvents);
        };
    }

    /**
     * Removes listener for events.
     *
     * Example:
     * ```cpp
     * auto id = sub->addEventListener([](auto){});
     *
     * sub->removeEventListener(id);
     * ```
     *
     * @param listenerId The listener id
     */
    void removeEventListener(std::size_t listenerId) noexcept { onEvent_ -= listenerId; }

    /**
     * Returns a reference to an incoming events' handler (delegate), to which listeners can be added and removed.
     * Listener can be callable with signature: `void(const std::vector<std::shared_ptr<EventType>&)`
     *
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::Type, dxfcpp::TimeAndSale::Type});
     * auto id = sub->onEvent() += [](auto &&events) {
     *     for (const auto &e : events) {
     *         if (auto quote = e->template sharedAs<dxfcpp::Quote>(); quote) {
     *             std::cout << "Q   : " + quote->toString() << std::endl;
     *         } else if (auto tns = e->template sharedAs<dxfcpp::TimeAndSale>(); tns) {
     *             std::cout << "TnS : " + tns->toString() << std::endl;
     *         }
     *     }
     * };
     *
     * sub->addSymbols({"$TOP10L/Q", "$SP500#45", "$TICK", "SPX"});
     * sub->onEvent() -= id;
     * ```
     *
     * @return The incoming events' handler (delegate)
     */
    auto &onEvent() noexcept { return onEvent_; }

    /**
     * Adds the specified symbol to the set of subscribed symbols.
     * This is a convenience method to subscribe to one symbol at a time that has a return fast-path for a case when
     * the symbol is already in the set.
     * When subscribing to multiple symbols at once it is preferable to use @ref ::addSymbols(const SymbolsCollection
     * &collection) "addSymbols(symbols)" method.
     *
     * Example:
     * ```cpp
     * sub->addSymbols("TSLA");
     * sub->addSymbols("XBT/USD:GDAX"s);
     * sub->addSymbols("BTC/EUR:CXBITF"sv);
     * ```
     *
     * @param symbolWrapper The symbol.
     */
    void addSymbols(const SymbolWrapper &symbolWrapper) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::addSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
        }

        addSymbolImpl(symbolWrapper.toGraal());
    }

    /**
     * Removes the specified symbol from the set of subscribed symbols.
     * To conveniently remove one or few symbols you can use @ref ::removeSymbols(const SymbolsCollection &collection)
     * "removeSymbols(symbols)" method.
     *
     * Example:
     * ```cpp
     * sub->removeSymbols("TSLA");
     * sub->removeSymbols("XBT/USD:GDAX"s);
     * sub->removeSymbols("BTC/EUR:CXBITF"sv);
     * ```
     *
     * @param symbolWrapper The symbol.
     */
    void removeSymbols(const SymbolWrapper &symbolWrapper) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::removeSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
        }

        removeSymbolImpl(symbolWrapper.toGraal());
    }

    /**
     * Adds the specified collection (using iterators) of symbols to the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->addSymbols(v.begin(), v.end());
     * ```
     *
     * @tparam SymbolIt The collection's iterator type
     * @param begin The beginning of the collection of symbols.
     * @param end The end of symbol collection.
     */
    template <typename SymbolIt> void addSymbols(SymbolIt begin, SymbolIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::addSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        std::for_each(
#if __cpp_lib_parallel_algorithm
            std::execution::par,
#endif
            begin, end, [this](const SymbolWrapper &wrapper) { addSymbolImpl(wrapper.toGraal()); });
    }

    /**
     * Adds the specified collection of symbols to the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->addSymbols(std::vector{"AAPL", "IBM"});
     * sub->addSymbols(v);
     * ```
     *
     * @tparam SymbolsCollection The symbols collection's type
     * @param collection The symbols collection
     */
    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void addSymbols(const SymbolsCollection &collection) noexcept {
        addSymbols(std::begin(collection), std::end(collection));
    }

    /**
     * Adds the specified collection (initializer list) of symbols to the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * sub->addSymbols({"AAPL", "IBM"sv, "TSLA"s, "GOOG"_s});
     * ```
     *
     * @param collection The symbols collection
     */
    void addSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        addSymbols(collection.begin(), collection.end());
    }

    /**
     * Removes the specified collection (using iterators) of symbols from the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->removeSymbols(v.begin(), v.end());
     * ```
     *
     * @tparam SymbolIt The collection's iterator type
     * @param begin The beginning of the collection of symbols.
     * @param end The end of symbol collection.
     */
    template <typename SymbolIt> void removeSymbols(SymbolIt begin, SymbolIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::removeSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        std::for_each(
#if __cpp_lib_parallel_algorithm
            std::execution::par,
#endif
            begin, end, [this](const SymbolWrapper &wrapper) { removeSymbolImpl(wrapper.toGraal()); });
    }

    /**
     * Removes the specified collection of symbols from the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->removeSymbols(std::vector{"AAPL", "IBM"});
     * sub->removeSymbols(v);
     * ```
     *
     * @tparam SymbolsCollection The symbols collection's type
     * @param collection The symbols collection
     */
    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void removeSymbols(SymbolsCollection &&collection) noexcept {
        removeSymbols(std::begin(collection), std::end(collection));
    }

    /**
     * Removes the specified collection (initializer list) of symbols from the set of subscribed symbols.
     *
     * Example:
     * ```cpp
     * sub->removeSymbols({"AAPL", "IBM"sv, "TSLA"s, "GOOG"_s});
     * ```
     *
     * @param collection The symbols collection
     */
    void removeSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        removeSymbols(collection.begin(), collection.end());
    }

    /**
     * Changes the set of subscribed symbols so that it contains just the symbols from the specified collection (using
     * iterators).
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->setSymbols(v.begin(), v.end());
     * ```
     *
     * @tparam SymbolIt The collection's iterator type
     * @param begin The beginning of the collection of symbols.
     * @param end The end of symbol collection.
     */
    template <typename SymbolIt> void setSymbols(SymbolIt begin, SymbolIt end) noexcept {
        // TODO: implement using the native implementation
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::setSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        clearImpl();
        addSymbols(begin, end);
    }

    /**
     * Changes the set of subscribed symbols so that it contains just the symbols from the specified collection.
     *
     * Example:
     * ```cpp
     * auto v = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};
     *
     * sub->setSymbols(std::vector{"AAPL", "IBM"});
     * sub->setSymbols(v);
     * ```
     *
     * @tparam SymbolsCollection The symbols collection's type
     * @param collection The symbols collection
     */
    template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
    void setSymbols(SymbolsCollection &&collection) noexcept {
        setSymbols(std::begin(collection), std::end(collection));
    }

    /**
     * Changes the set of subscribed symbols so that it contains just the symbols from the specified collection
     * (initializer list).
     *
     * Example:
     * ```cpp
     * sub->setSymbols({"AAPL", "IBM"sv, "TSLA"s, "GOOG"_s});
     * ```
     *
     * @param collection The symbols collection
     */
    void setSymbols(std::initializer_list<SymbolWrapper> collection) noexcept {
        setSymbols(collection.begin(), collection.end());
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
    const std::unordered_set<EventTypeEnum> &getEventTypes() const noexcept { return eventTypes_; }

    /**
     * Returns `true` if this subscription contains the corresponding event type.
     *
     * @return `true` if this subscription contains the corresponding event type.
     *
     * @see ::getEventTypes()
     */
    bool containsEventType(const EventTypeEnum &eventType) const noexcept { return eventTypes_.contains(eventType); }

    /**
     * Returns a set of subscribed symbols. The resulting set maybe either a snapshot of the set of
     * the subscribed symbols at the time of invocation or a weakly consistent view of the set.
     *
     * @return A set of subscribed symbols.
     */
    auto getSymbols() noexcept;

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