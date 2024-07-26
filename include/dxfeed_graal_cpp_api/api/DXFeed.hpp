// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../promise/Promise.hpp"

#include "DXFeedSubscription.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXEndpoint;
class EventTypeEnum;

/**
 * Main entry class for dxFeed API (<b>read it first</b>).
 *
 * <h3>Sample usage</h3>
 *
 * This section gives sample usage scenarios.
 *
 * <h4>Default singleton instance</h4>
 *
 * There is a singleton instance of the feed that is returned by DXFeed::getInstance() method.
 * It is created on the first use with default configuration properties that are explained in detail in
 * documentation for DXEndpoint class in the "Default properties" section.
 *
 * <p>In particular,
 * you can provide a default address to connect and credentials  using
 * "@ref DXEndpoint::DXFEED_ADDRESS_PROPERTY "dxfeed.address"",
 * "@ref DXEndpoint::DXFEED_USER_PROPERTY "dxfeed.user"", and
 * "@ref DXEndpoint::DXFEED_PASSWORD_PROPERTY "dxfeed.password""
 * system properties or by putting them into
 * "@ref DXEndpoint::DXFEED_PROPERTIES_PROPERTY "dxfeed.properties""
 * file in the same directory. dxFeed API samples come with a ready-to-use "<b>dxfeed.properties</b>"
 * file that contains an address of dxFeed demo feed at "<b>demo.dxfeed.com:7300</b>" and
 * demo access credentials.
 *
 * <h4>Subscribe for single event type</h4>
 *
 * The following code creates listener that prints mid price for each quote
 * and subscribes for quotes on SPDR S&P 500 ETF symbol:
 * <pre><tt>
 * auto sub = @ref DXFeed "DXFeed"::@ref DXFeed::getInstance() "getInstance"()->@ref DXFeed::createSubscription()
 * "createSubscription"(Quote::TYPE);
 *
 * sub->@ref DXFeedSubscription::addEventListener() "addEventListener"<Quote>([](const auto& quotes) {
 *     for (const auto& quote : quotes) {
 *         std::cout << "Mid = " + (quote->@ref Quote::getBidPrice() "getBidPrice"() + quote->@ref Quote::getAskPrice()
 * "getAskPrice"()) / 2) << std::endl;
 *     }
 * });
 *
 * sub->@ref DXFeedSubscription::addSymbols() "addSymbols"("SPY");</tt></pre>
 *
 * Note, that order of calls is important here. By attaching listeners first and then setting
 * subscription we ensure that the current quote gets received by the listener. See DXFeedSubscription::addSymbols() for
 * details. If a set of symbols is changed first, then @ref DXFeedSubscription::addEventListener()
 * "sub->addEventListener" raises an IllegalStateException in JVM to protected from hard-to-catch bugs with potentially
 * missed events.
 *
 * <h4>Subscribe for multiple event types</h4>
 *
 * The following code creates listener that prints each received event and
 * subscribes for quotes and trades on SPDR S&P 500 ETF symbol:
 * <pre><tt>
 * auto sub = @ref DXFeed "DXFeed"::@ref DXFeed::getInstance() "getInstance"()->@ref DXFeed::createSubscription()
 * "createSubscription"({Quote::TYPE, Trade::TYPE});
 *
 * sub->@ref DXFeedSubscription::addEventListener() "addEventListener"([](auto&& events) {
 *     for (const auto& event : events) {
 *         std::cout << event << std::endl;
 *     }
 * });
 *
 * sub->@ref DXFeedSubscription::addSymbols() "addSymbols"("SPY");</tt></pre>
 *
 * <h4>Subscribe for event and query periodically its last value</h4>
 *
 * The following code subscribes for trades on SPDR S&P 500 ETF symbol and
 * prints last trade every second.
 *
 * <pre><tt>
 * using namespace std::chrono_literals;
 *
 * auto sub = @ref DXFeed "DXFeed"::@ref DXFeed::getInstance() "getInstance"()->@ref DXFeed::createSubscription()
 * "createSubscription"({Trade::TYPE});
 *
 * sub->@ref DXFeedSubscription::addSymbols() "addSymbols"("SPY");
 *
 * auto feed = @ref DXFeed "DXFeed"::@ref DXFeed::getInstance() "getInstance"();
 *
 * while (true) {
 *     std::cout << System.out.println(feed->@ref DXFeed::getLastEvent() "getLastEvent"(Trade::create("SPY")));
 *     std::this_thread::sleep_for(1000ms);
 * }</tt></pre>
 *
 * <h3>Threads and locks</h3>
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 */
struct DXFCPP_EXPORT DXFeed : SharedEntity {
    /// The alias to a type of shared pointer to the DXFeed object
    using Ptr = std::shared_ptr<DXFeed>;

    /// The alias to a type of unique pointer to the DXFeed object
    using Unique = std::unique_ptr<DXFeed>;

    friend struct DXEndpoint;

  private:
    JavaObjectHandle<DXFeed> handle_;
    static std::shared_ptr<DXFeed> create(void *feedHandle);

    void *getTimeSeriesPromiseImpl(const EventTypeEnum &eventType, const SymbolWrapper &symbol, std::int64_t fromTime,
                                   std::int64_t toTime) const;

    std::shared_ptr<EventType> getLastEventIfSubscribedImpl(const EventTypeEnum &eventType,
                                                            const SymbolWrapper &symbol) const;

  protected:
    DXFeed() noexcept : handle_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeed()");
        }
    }

  public:
    ~DXFeed() noexcept override {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXFeed{" + handle_.toString() + "}::~DXFeed()");
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
    static std::shared_ptr<DXFeed> getInstance();

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
    void attachSubscription(std::shared_ptr<DXFeedSubscription> subscription);

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
    void detachSubscription(std::shared_ptr<DXFeedSubscription> subscription);

    /**
     * Detaches the given subscription from this feed and clears data delivered to this subscription
     * by publishing empty events. This method does nothing if the
     * corresponding subscription is not attached to this feed.
     *
     * @param subscription The subscription.
     * @see DXFeed::detachSubscription()
     */
    void detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription);

    /**
     * Returns the last event for the specified event instance.
     * This method works only for event types that implement LastingEvent marker interface.
     * This method <b>does not</b> make any remote calls to the uplink data provider.
     * It just retrieves last received event from the local cache of this feed.
     * The events are stored in the cache only if there is some attached DXFeedSubscription that is subscribed to the
     * corresponding symbol and event type.
     * WildcardSymbol::ALL subscription does not count for that purpose.
     *
     * <p>Use @ref ::getLastEventPromise() "getLastEventPromise" method if an event needs to be requested in the absence
     * of subscription.
     *
     * <p> This method fills in the values for the last event into the `event argument.
     * If the last event is not available for any reason (no subscription, no connection to uplink, etc.)
     * then the event object is not changed.
     * This method always returns the same `event` instance that is passed to it as an argument.
     *
     * <p>This method provides no way to distinguish a case when there is no subscription from the case when
     * there is a subscription, but the event data have not arrived yet. It is recommended to use
     * @ref ::getLastEventIfSubscribed() "getLastEventIfSubscribed" method instead of this `getLastEvent` method to
     * fail-fast in case when the subscription was supposed to be set by the logic of the code, since
     * @ref ::getLastEventIfSubscribed() "getLastEventIfSubscribed" method returns null when there is no subscription.
     *
     * <p>Note, that this method does not work when DXEndpoint was created with
     * @ref DXEndpoint::Role::STREAM_FEED "STREAM_FEED" role (never fills in the event).
     *
     * @tparam E The type of event.
     * @param event The event.
     * @return The same event.
     */
    template <Derived<LastingEvent> E> std::shared_ptr<E> getLastEvent(std::shared_ptr<E> event) {
        if (auto last = getLastEventIfSubscribed<E>(event->getEventSymbol())) {
            event->assign(last);
        }

        return event;
    }

    /**
     * Returns the last events for the specified list of event instances.
     * This is a bulk version of @ref ::getLastEvent() "getLastEvent" method.
     *
     * <p>Note, that this method does not work when DXEndpoint was created with
     * @ref DXEndpoint::Role::STREAM_FEED "STREAM_FEED" role.
     *
     * @tparam Collection The collection type.
     * @param events The collection of shared ptrs of events.
     * @return The same collection of shared ptrs of events.
     */
    template <typename Collection, typename Element = std::decay_t<decltype(std::begin(Collection()))>,
              typename Event = std::decay_t<decltype(*Element())>>
    const Collection &getLastEvents(const Collection &events) {
        static_assert(
            std::is_same_v<Element, std::shared_ptr<Event>> && std::is_base_of_v<LastingEvent, Event>,
            "The collection element must be of type `std::shared_ptr<Event>`, where `Event` is a descendant of "
            "`LastingEvent`");

        for (auto e : events) {
            getLastEvent(e);
        }

        return events;
    }

    /**
     * Returns the last event for the specified event type and symbol if there is a subscription for it.
     * This method works only for event types that implement LastingEvent marker interface.
     * This method <b>does not</b> make any remote calls to the uplink data provider.
     * It just retrieves last received event from the local cache of this feed.
     * The events are stored in the cache only if there is some attached DXFeedSubscription that is subscribed to the
     * corresponding event type and symbol.
     * The subscription can also be permanently defined using DXEndpoint properties.
     * WildcardSymbol::ALL subscription does not count for that purpose.
     * If there is no subscription, then this method returns `std::shared_ptr<E>(nullptr)`.
     *
     * <p>If there is a subscription, but the event has not arrived from the uplink data provider,
     * this method returns an non-initialized event object: its @ref EventType#getEventSymbol() "eventSymbol"
     * property is set to the requested symbol, but all the other properties have their default values.
     *
     * <p>Use @ref ::getLastEventPromise() "getLastEventPromise" method if an event needs to be requested in the
     * absence of subscription.
     *
     * <p>Note, that this method does not work when DXEndpoint} was created with @ref DXEndpoint::Role::STREAM_FEED
     * "STREAM_FEED" role (always returns `std::shared_ptr<E>(nullptr)`).
     *
     * @tparam E The type of event.
     * @param symbol The symbol.
     * @return the event or `std::shared_ptr<E>(nullptr)` if there is no subscription for the specified event type and
     * symbol.
     */
    template <Derived<LastingEvent> E> std::shared_ptr<E> getLastEventIfSubscribed(const SymbolWrapper &symbol) {
        return getLastEventIfSubscribedImpl(E::TYPE, symbol)->template sharedAs<E>();
    }

    /**
     * Creates new subscription for a single event type that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link DXFeed::attachSubscription.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription(dxfcpp::Quote::TYPE);
     * ```
     *
     * @param eventType The type of event
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(const EventTypeEnum &eventType);

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link DXFeed::attachSubscription.
     *
     * Example:
     * ```cpp
     * auto eventTypes = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};
     *
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription(eventTypes.begin(), eventTypes.end());
     * ```
     *
     * ```cpp
     * std::vector types{dxfcpp::Quote::TYPE, dxfcpp::Trade::TYPE, dxfcpp::Summary::TYPE};
     *
     * auto sub = dxfcpp::DXFeedSubscription::create(types.begin(), types.end());
     * ```
     *
     * ```cpp
     * std::set types{dxfcpp::Quote::TYPE, dxfcpp::Trade::TYPE, dxfcpp::Summary::TYPE};
     * auto endpoint = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED)->build();
     * auto sub = endpoint->getFeed()->createSubscription(eventTypes.begin(), eventTypes.end());
     *
     * endpoint->connect("demo.dxfeed.com:7300");
     * ```
     *
     * @tparam EventTypeIt The iterator type of the collection of event types
     * @param begin The start iterator
     * @param end The end iterator
     * @return The new subscription
     */
    template <typename EventTypeIt>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypeIt begin, EventTypeIt end) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("{}::createSubscription(eventTypes = " + namesToString(begin, end) + ")");
        }

        auto sub = DXFeedSubscription::create(begin, end);

        attachSubscription(sub);

        return sub;
    }

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link DXFeed::attachSubscription.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
     * ```
     *
     * @param eventTypes The initializer list of event types
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(std::initializer_list<EventTypeEnum> eventTypes);

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link DXFeed::attachSubscription.
     *
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription(std::unordered_set{dxfcpp::Quote::TYPE,
     * dxfcpp::TimeAndSale::TYPE});
     * ```
     *
     * ```cpp
     * std::vector types = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription(types);
     * ```
     *
     * @tparam EventTypesCollection The class of the collection of event types
     * @param eventTypes The collection of event types
     * @return The new subscription
     */
    template <typename EventTypesCollection>
    std::shared_ptr<DXFeedSubscription> createSubscription(EventTypesCollection &&eventTypes) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::createSubscription(eventTypes = " +
                            namesToString(std::begin(eventTypes), std::end(eventTypes)) + ")");
        }

        auto sub = DXFeedSubscription::create(eventTypes);

        attachSubscription(sub);

        return sub;
    }

    /**
     * Requests time series of events for the specified event type, symbol, and a range of time.
     *
     * @tparam E The type of event.
     * @param symbol The symbol.
     * @param fromTime The time, inclusive, to request events from (see TimeSeriesEvent::getTime()).
     * @param toTime The time, inclusive, to request events to (see TimeSeriesEvent::getTime()).
     *               Use `std::numeric_limits<std::int64_t>::max()` or `LLONG_MAX` macro to retrieve events without an
     * upper limit on time.
     * @return The promise for the result of the request.
     */
    template <Derived<TimeSeriesEvent> E>
    Promise<std::vector<std::shared_ptr<E>>> getTimeSeriesPromise(const SymbolWrapper &symbol, std::int64_t fromTime,
                                                                  std::int64_t toTime) const {
        return Promise<std::vector<std::shared_ptr<E>>>(getTimeSeriesPromiseImpl(E::TYPE, symbol, fromTime, toTime));
    }

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()