// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "osub/ObservableSubscription.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/EventClassList.hpp"
#include "../internal/context/ApiContext.hpp"

#include "../entity/EntityModule.hpp"
#include "../event/EventType.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../symbols/SymbolWrapper.hpp"

#include "../util/TimePeriod.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;
struct MarketEvent;
struct IndexedEvent;
struct TimeSeriesEvent;
struct LastingEvent;

/**
 * Subscription for a set of symbols and event types.
 */
class DXFCPP_EXPORT DXFeedSubscription : public RequireMakeShared<DXFeedSubscription>, public ObservableSubscription {
  public:
    static constexpr std::size_t FAKE_CHANGE_LISTENER_ID{static_cast<std::size_t>(-1)};

    ///
    using OnEventHandler = SimpleHandler<void(const std::vector<std::shared_ptr<EventType>> &)>;

    // These constants are linked with same ones in RecordBuffer - POOLED_CAPACITY and UNLIMITED_CAPACITY.
    /**
     * The optimal events' batch limit for single notification in OnEventHandler.
     */
    static const std::int32_t OPTIMAL_BATCH_LIMIT = 0;

    /**
     * The maximum events' batch limit for single notification in OnEventHandler.
     */
    static const std::int32_t MAX_BATCH_LIMIT = std::numeric_limits<std::int32_t>::max();

  private:
    friend struct DXFeed;

    inline static std::atomic<std::size_t> lastChangeListenerId_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;

    std::unordered_set<EventTypeEnum> eventTypes_;
    JavaObjectHandle<DXFeedSubscription> handle_;

    std::mutex eventListenerMutex_{};
    JavaObjectHandle<DXFeedEventListener> eventListenerHandle_;
    OnEventHandler onEvent_{};
    std::unordered_map<std::size_t, std::shared_ptr<ObservableSubscriptionChangeListener>> changeListeners_;
    std::recursive_mutex changeListenersMutex_{};

    static JavaObjectHandle<DXFeedSubscription>
    createSubscriptionHandleFromEventClassList(const std::unique_ptr<EventClassList> &list);

    void setEventListenerHandle(Id<DXFeedSubscription> id);

    bool tryToSetEventListenerHandle();

    void setSymbolsImpl(void *graalSymbolList) const;
    void addSymbolsImpl(void *graalSymbolList) const;
    void removeSymbolsImpl(void *graalSymbolList) const;

  public:
    /// The alias to a type of shared pointer to the DXFeedSubscription object
    using Ptr = std::shared_ptr<DXFeedSubscription>;

    /// The alias to a type of unique pointer to the DXFeedSubscription object
    using Unique = std::unique_ptr<DXFeedSubscription>;

    DXFeedSubscription(LockExternalConstructionTag);

    DXFeedSubscription(LockExternalConstructionTag, const EventTypeEnum &eventType);

    template <typename EventTypeIt>
#if __cpp_concepts
        requires requires(EventTypeIt iter) {
            { *iter } -> dxfcpp::ConvertibleTo<EventTypeEnum>;
        }
#endif
    DXFeedSubscription(LockExternalConstructionTag tag, EventTypeIt begin, EventTypeIt end) : DXFeedSubscription{tag} {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug("DXFeedSubscription(eventTypes = " + namesToString(begin, end) + ")");
        }

        eventTypes_ = std::unordered_set<EventTypeEnum>{begin, end};

        auto list = EventClassList::create(eventTypes_.begin(), eventTypes_.end());

        handle_ = createSubscriptionHandleFromEventClassList(list);
    }

    DXFeedSubscription(LockExternalConstructionTag tag, std::initializer_list<EventTypeEnum> eventTypes)
        : DXFeedSubscription(tag, eventTypes.begin(), eventTypes.end()) {
    }

    template <typename EventTypesCollection>
    explicit DXFeedSubscription(LockExternalConstructionTag tag, EventTypesCollection &&eventTypes)
#if __cpp_concepts
        requires requires {
            {
                DXFeedSubscription(tag, std::begin(std::forward<EventTypesCollection>(eventTypes)),
                                   std::end(std::forward<EventTypesCollection>(eventTypes)))
            };
        }
#endif
        : DXFeedSubscription(tag, std::begin(std::forward<EventTypesCollection>(eventTypes)),
                             std::end(std::forward<EventTypesCollection>(eventTypes))) {
    }

    ///
    std::string toString() const override;

    ~DXFeedSubscription() override;

    /**
     * Creates <i>detached</i> subscription for a single event type.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription::create(dxfcpp::Quote::TYPE);
     * ```
     *
     * @param eventType the event type.
     */
    static std::shared_ptr<DXFeedSubscription> create(const EventTypeEnum &eventType);

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * Example:
     * ```cpp
     * auto eventTypes = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};
     *
     * auto sub = dxfcpp::DXFeedSubscription::create(eventTypes.begin(), eventTypes.end());
     * ```
     *
     * ```cpp
     * std::vector types{dxfcpp::Quote::TYPE, dxfcpp::Trade::TYPE, dxfcpp::Summary::TYPE};
     *
     * auto sub = dxfcpp::DXFeedSubscription::create(types.begin(), types.end());
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
            { *iter } -> dxfcpp::ConvertibleTo<EventTypeEnum>;
        }
#endif
    static std::shared_ptr<DXFeedSubscription> create(EventTypeIt begin, EventTypeIt end) {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug("DXFeedSubscription::create(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto sub = createShared(begin, end);
        auto id = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->registerEntity(sub);

        dxfcpp::ignore_unused(id);

        return sub;
    }

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription::create({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
     * ```
     *
     * @param eventTypes The event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    static std::shared_ptr<DXFeedSubscription> create(std::initializer_list<EventTypeEnum> eventTypes);

    /**
     * Creates <i>detached</i> subscription for the given collection of event types.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeedSubscription::create(std::unordered_set{dxfcpp::Quote::TYPE,
     * dxfcpp::TimeAndSale::TYPE});
     * ```
     *
     * ```cpp
     * std::vector types = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};
     * auto sub = dxfcpp::DXFeedSubscription::create(types);
     * ```
     *
     * @tparam EventTypesCollection The type of the collection of event types
     * @param eventTypes The event type collection.
     * @return The new <i>detached</i> subscription for the given collection of event types.
     */
    template <typename EventTypesCollection>
    static std::shared_ptr<DXFeedSubscription> create(EventTypesCollection &&eventTypes) {
        auto sub = createShared(std::forward<EventTypesCollection>(eventTypes));
        auto id = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->registerEntity(sub);

        dxfcpp::ignore_unused(id);

        return sub;
    }

    /**
     * Attaches subscription to the specified feed.
     *
     * @param feed The feed to attach to.
     */
    void attach(std::shared_ptr<DXFeed> feed);

    /**
     * Detaches subscription from the specified feed.
     *
     * @param feed The feed to detach from.
     */
    void detach(std::shared_ptr<DXFeed> feed);

    /**
     * Returns `true` if this subscription is closed.
     *
     * @return `true` if this subscription is closed.
     *
     * @see DXFeedSubscription::close()
     */
    bool isClosed() override;

    /**
     * Closes this subscription and makes it <i>permanently detached</i>.
     * This method notifies all installed instances of subscription change listeners by invoking `subscriptionClosed`
     * while holding the lock for this subscription. This method clears lists of all installed
     * event listeners and subscription change listeners and makes sure that no more listeners
     * can be added.
     */
    void close() const;

    /**
     * Returns a set of subscribed event types.
     *
     * @return A set of subscribed event types.
     */
    std::unordered_set<EventTypeEnum> getEventTypes() override;

    /**
     * Returns `true` if this subscription contains the corresponding event type.
     *
     * @param eventType The type of event that is checked.
     * @return `true` if this subscription contains the corresponding event type.
     *
     * @see DXFeedSubscription::getEventTypes()
     */
    bool containsEventType(const EventTypeEnum &eventType) override;

    /**
     * Clears the set of subscribed symbols.
     */
    void clear() const;

    /**
     * Returns a set of subscribed symbols (depending on the actual implementation of subscription).
     *
     * The resulting set maybe either a snapshot of the set of the subscribed symbols at the time of invocation or a
     * weakly consistent view of the set.
     *
     * @return A set of subscribed symbols.
     */
    std::vector<SymbolWrapper> getSymbols() const;

    /**
     * Returns a set of decorated symbols (depending on the actual implementation of subscription).
     *
     * The resulting set maybe either a snapshot of the set of the subscribed symbols at the time of invocation or a
     * weakly consistent view of the set.
     *
     * @return A set of decorated subscribed symbols.
     */
    std::vector<SymbolWrapper> getDecoratedSymbols() const;

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
    template <typename SymbolIt> void setSymbols(SymbolIt begin, SymbolIt end) const {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(toString() + "::setSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        auto *list = SymbolWrapper::SymbolListUtils::toGraalList(begin, end);

        setSymbolsImpl(list);
        SymbolWrapper::SymbolListUtils::freeGraalList(list);
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
    void setSymbols(SymbolsCollection &&collection) const {
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
    void setSymbols(std::initializer_list<SymbolWrapper> collection) const;

    /**
     * Adds the specified symbol to the set of subscribed symbols.
     * This is a convenience method to subscribe to one symbol at a time that has a return fast-path for a case when
     * the symbol is already in the set.
     * When subscribing to multiple symbols at once it is preferable to use @ref DXFeedSubscription::addSymbols(const
     * SymbolsCollection &collection) "addSymbols(symbols)" method.
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
    void addSymbols(const SymbolWrapper &symbolWrapper) const;

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
    template <typename SymbolIt> void addSymbols(SymbolIt begin, SymbolIt end) const {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(toString() + "::addSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        auto *list = SymbolWrapper::SymbolListUtils::toGraalList(begin, end);

        addSymbolsImpl(list);
        SymbolWrapper::SymbolListUtils::freeGraalList(list);
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
    void addSymbols(const SymbolsCollection &collection) const {
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
    void addSymbols(std::initializer_list<SymbolWrapper> collection) const;

    /**
     * Removes the specified symbol from the set of subscribed symbols.
     * To conveniently remove one or few symbols you can use @ref DXFeedSubscription::removeSymbols(const
     * SymbolsCollection &collection) "removeSymbols(symbols)" method.
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
    void removeSymbols(const SymbolWrapper &symbolWrapper) const;

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
    template <typename SymbolIt> void removeSymbols(SymbolIt begin, SymbolIt end) const {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(toString() + "::removeSymbols(symbols = " + elementsToString(begin, end) + ")");
        }

        auto *list = SymbolWrapper::SymbolListUtils::toGraalList(begin, end);

        removeSymbolsImpl(list);
        SymbolWrapper::SymbolListUtils::freeGraalList(list);
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
    void removeSymbols(SymbolsCollection &&collection) const {
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
    void removeSymbols(std::initializer_list<SymbolWrapper> collection) const;

    /**
     * Returns the aggregation period for data for this subscription instance.
     *
     * @return The aggregation period for data, represented as a TimePeriod object.
     */
    TimePeriod getAggregationPeriod() const;

    /**
     * Sets the aggregation period for data.
     * This method sets a new aggregation period for data, which will only take effect on the next iteration of
     * data notification. For example, if the current aggregation period is 5 seconds and it is changed
     * to 1 second, the next call to the next call to the retrieve method may take up to 5 seconds, after which
     * the new aggregation period will take effect.
     *
     * @param aggregationPeriod the new aggregation period for data
     */
    void setAggregationPeriod(const TimePeriod &aggregationPeriod) const;

    /**
     * Sets the aggregation period for data.
     * This method sets a new aggregation period for data, which will only take effect on the next iteration of
     * data notification. For example, if the current aggregation period is 5 seconds and it is changed
     * to 1 second, the next call to the next call to the retrieve method may take up to 5 seconds, after which
     * the new aggregation period will take effect.
     *
     * @param aggregationPeriod the new aggregation period (in millis) for data
     */
    void setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) const {
        return setAggregationPeriod(TimePeriod::valueOf(aggregationPeriod));
    }

    /**
     * Sets the aggregation period for data.
     * This method sets a new aggregation period for data, which will only take effect on the next iteration of
     * data notification. For example, if the current aggregation period is 5 seconds and it is changed
     * to 1 second, the next call to the next call to the retrieve method may take up to 5 seconds, after which
     * the new aggregation period will take effect.
     *
     * @param aggregationPeriod the new aggregation period (in millis) for data
     */
    void setAggregationPeriod(std::int64_t aggregationPeriod) const {
        return setAggregationPeriod(TimePeriod::valueOf(aggregationPeriod));
    }

    /**
     * Adds listener for events.
     * Event lister can be added only when subscription is not producing any events.
     * The subscription must be either empty
     * (its set of @ref DXFeedSubscription::getSymbols() "symbols" is empty or not @ref DXFeedSubscription::attach()
     * "attached" to any feed (its set of change listeners is empty).
     *
     * This method does nothing if this subscription is closed.
     *
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
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
    std::size_t addEventListener(EventListener &&listener)
#if __cpp_concepts
        requires requires {
            { listener(std::vector<std::shared_ptr<EventType>>{}) } -> std::same_as<void>;
        }
#endif
    {
        if (!tryToSetEventListenerHandle()) {
            return OnEventHandler::FAKE_ID;
        }

        return onEvent_ += listener;
    }

    /**
     * Adds typed listener for events.
     * Event lister can be added only when subscription is not producing any events.
     * The subscription must be either empty
     * (its set of @ref DXFeedSubscription::getSymbols() "symbols" is empty or not @ref DXFeedSubscription::attach()
     * "attached" to any feed (its set of change listeners is empty).
     *
     * This method does nothing if this subscription is closed.
     *
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
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
     * sub->addEventListener<dxfcpp::MarketEvent>([](const auto &marketEvents) -> void {
     *     for (const auto &me : marketEvents) {
     *         std::cout << "Market Event's symbol: " + me->getEventSymbol() << std::endl;
     *     }
     * });
     *
     * sub->addSymbols({"$TOP10L/Q", "AAPL", "$TICK", "SPX"});
     * ```
     *
     * @tparam EventT The event type (EventType's child with field TYPE, convertible to EventTypeEnum or MarketEvent
     * or LastingEvent or TimeSeriesEvent or IndexedEvent)
     * @param listener The listener. Listener can be callable with signature: `void(const
     * std::vector<std::shared_ptr<EventT>&)`
     * @return The listener id
     */
    template <typename EventT>
    std::size_t addEventListener(std::function<void(const std::vector<std::shared_ptr<EventT>> &)> &&listener)
#if __cpp_concepts
        requires std::is_base_of_v<EventType, EventT> &&
                 (requires {
                     { EventT::TYPE } -> dxfcpp::ConvertibleTo<EventTypeEnum>;
                 } || std::is_same_v<EventT, MarketEvent> || std::is_same_v<EventT, LastingEvent> ||
                  std::is_same_v<EventT, TimeSeriesEvent> || std::is_same_v<EventT, IndexedEvent>)
#endif
    {
        if (!tryToSetEventListenerHandle()) {
            return SimpleHandler<void(const std::vector<std::shared_ptr<EventType>> &)>::FAKE_ID;
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
    void removeEventListener(std::size_t listenerId);

    /**
     * Returns a reference to an incoming events' handler (delegate), to which listeners can be added and removed.
     * Listener can be callable with signature: `void(const std::vector<std::shared_ptr<EventType>&)`
     *
     * Example:
     * ```cpp
     * auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
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
    OnEventHandler &onEvent();

    std::size_t addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) override;

    void removeChangeListener(std::size_t changeListenerId) override;

    /**
     * @return maximum number of events in the single notification of OnEventHandler.
     * Special cases are supported for constants ::OPTIMAL_BATCH_LIMIT and ::MAX_BATCH_LIMIT.
     */
    std::int32_t getEventsBatchLimit() const;

    /**
     * Sets maximum number of events in the single notification of OnEventHandler.
     * Special cases are supported for constants ::OPTIMAL_BATCH_LIMIT and ::MAX_BATCH_LIMIT.
     *
     * @param eventsBatchLimit the notification events limit
     * @throws JavaException if eventsBatchLimit < 0 (see ::OPTIMAL_BATCH_LIMIT or ::MAX_BATCH_LIMIT)
     */
    void setEventsBatchLimit(std::int32_t eventsBatchLimit) const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()