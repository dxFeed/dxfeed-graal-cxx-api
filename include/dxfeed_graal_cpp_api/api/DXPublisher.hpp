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

#include "DXFeedSubscription.hpp"

#include "../event/EventMapper.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXEndpoint;
class EventTypeEnum;
struct ObservableSubscription;

/**
 * Provides API for publishing of events to local or remote DXFeed.
 *
 * <h3>Sample usage</h3>
 *
 * This section gives sample usage scenarios.
 *
 * <h4>Default singleton instance</h4>
 *
 * There is a singleton instance of the publisher that is returned by DXPublisher::getInstance() method.
 * It is created on the first use with default configuration properties that are explained in detail in
 * documentation for DXEndpoint class in the "Default properties" section. In particular,
 * you can provide a default address to connect using
 * '@ref DXEndpoint::DXPUBLISHER_ADDRESS_PROPERTY "dxpublisher.address"'
 * system property or by putting it into
 * '@ref DXEndpoint::DXPUBLISHER_PROPERTIES_PROPERTY "dxpublisher.properties"'
 * file in the current directory.
 *
 * <h4>Publish a single event</h4>
 *
 * The following code publishes a single quote for a "A:TEST" symbol:
 * <pre><tt>
 * auto @ref Quote "quote" = std::make_shared<@ref Quote "Quote">("A:TEST");
 * quote->@ref Quote::setBidPrice "setBidPrice"(100);
 * quote->@ref Quote::setAskPrice "setAskPrice"(101);
 * @ref DXPublisher "DXPublisher"::@ref DXPublisher::getInstance() "getInstance"()->@ref DXPublisher::publishEvents()
 * "publishEvents"(quote);</tt></pre>
 *
 * <h3>Threads and locks</h3>
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 */
struct DXFCPP_EXPORT DXPublisher : SharedEntity {
    /// The alias to a type of shared pointer to the DXPublisher object
    using Ptr = std::shared_ptr<DXPublisher>;

    /// The alias to a type of unique pointer to the DXPublisher object
    using Unique = std::unique_ptr<DXPublisher>;

    friend struct DXEndpoint;

  private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<DXPublisher> handle_;
    std::shared_ptr<ObservableSubscription> subscription_{};

    static std::shared_ptr<DXPublisher> create(void *handle);
    void publishEventsImpl(void *graalEventsList) const noexcept;

  protected:
    DXPublisher() noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXPublisher()");
        }
    }

  public:
    ~DXPublisher() noexcept override {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXPublisher{" + handle_.toString() + "}::~DXPublisher()");
        }
    }

    /**
     * Returns a default application-wide singleton instance of DXPublisher. Most applications use only a single
     * data-sink and should rely on this method to get one. This is a shortcut to
     * @ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::getInstance() "getInstance"(DXEndpoint::Role::PUBLISHER)->@ref
     * DXEndpoint::getPublisher() "getPublisher"().
     */
    static std::shared_ptr<DXPublisher> getInstance();

    /**
     * Publishes events to the corresponding feed. If the @ref DXEndpoint "endpoint" of this publisher has
     * @ref DXEndpoint::getRole() "role" of DXEndpoint::Role::PUBLISHER and it is connected, the
     * published events will be delivered to the remote endpoints. Local @ref DXEndpoint::getFeed() "feed" will
     * always receive published events.
     *
     * <p> This method serializes all events into internal representation, so that the instance of the collection as
     * well as the instances of events can be reused after invocation of this method returns.
     *
     * <p> DXFeed instances that are connected to this publisher either locally or via network receive published
     * events if and only if they are subscribed to the corresponding symbols, or they are subscribed via
     * WildcardSymbol::ALL, or, in case of TimeSeriesEvent type, they are subscribed via DXFeedTimeSeriesSubscription
     * for the corresponding symbol and time frame.
     *
     * <p> Published events are not stored and get immediately lost if there is no subscription.
     * Last published events of LastingEvent types are cached as long as subscription to them is maintained via a
     * specific event symbol (WildcardSymbol::ALL does not count) and the cache is discarded as soon as subscription
     * disappears.
     *
     * Example:
     *
     * ```cpp
     * using namespace dxfcpp;
     * using namespace dxfcpp::literals;
     * using namespace std::literals;
     *
     * System::setProperty("dxendpoint.eventTime", "true");
     * System::setProperty("dxscheme.nanoTime", "true");
     *
     * auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
     * auto feed = endpoint->getFeed();
     * auto publisher = endpoint->getPublisher();
     * auto sub = feed->createSubscription({Quote::TYPE});
     * auto quote = std::make_shared<Quote>("AAPL");
     *
     * quote->setEventTime(1692975409000);
     * quote->setTimeNanoPart(123);
     * quote->setSequence(123);
     * quote->setBidTime(1692975409000);
     * quote->setBidExchangeCode('B');
     * quote->setBidPrice(176.08);
     * quote->setBidSize(1.0);
     * quote->setAskTime(1692975409000);
     * quote->setAskExchangeCode('A');
     * quote->setAskPrice(176.16);
     * quote->setAskSize(2.0);
     *
     * sub->addEventListener([](const auto &events) {
     *     for (const auto &e : events) {
     *         if (const auto &q = e->template sharedAs<Quote>(); q) {
     *             std::cout << q << std::endl;
     *         }
     *     }
     * });
     *
     * sub->addSymbols("AAPL");
     * publisher->publishEvents(quote);
     * publisher->publishEvents({quote, quote});
     *
     * std::vector<EventType::Ptr> events{};
     *
     * events.push_back(quote);
     * events.push_back(quote);
     * publisher->publishEvents(events);
     * std::this_thread::sleep_for(1000ms);
     * endpoint->close();
     * ```
     *
     * @tparam EventsCollection The type of events collection (for example: std::vector<::EventType::Ptr>)
     * @param events The collection of events to publish.
     */
    template <typename EventsCollection>
    void publishEvents(EventsCollection &&events) noexcept
#if __cpp_concepts
        requires requires {
            { std::begin(std::forward<EventsCollection>(events)) };

            { std::end(std::forward<EventsCollection>(events)) };
        }
#endif
    {
        this->template publishEvents<decltype(std::begin(std::forward<EventsCollection>(events)))>(
            std::begin(std::forward<EventsCollection>(events)), std::end(std::forward<EventsCollection>(events)));
    }

    /**
     * Publishes events to the corresponding feed.
     *
     * @param events The collection of events to publish.
     */
    void publishEvents(std::initializer_list<std::shared_ptr<EventType>> events) noexcept {
        publishEvents(std::begin(events), std::end(events));
    }

    /**
     * Publishes an event to the corresponding feed.
     *
     * @param event The event to publish.
     */
    void publishEvents(std::shared_ptr<EventType> event) noexcept {
        publishEvents({event});
    }

    /**
     * Publishes events to the corresponding feed.
     *
     * @tparam EventIt The collection's iterator type
     * @param begin The beginning of the collection of events.
     * @param end The end of events collection.
     */
    template <typename EventIt> void publishEvents(EventIt begin, EventIt end) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(toString() + "::publishEvents(events = " + elementsToString(begin, end) + ")");
        }

        auto *list = EventMapper::toGraalList(begin, end);

        publishEventsImpl(list);
        EventMapper::freeGraalList(list);
    }

    /**
     * Returns observable set of subscribed symbols for the specified event type.
     * Note, that subscription is represented by SymbolWrapper symbols. Check the type of each symbol
     * in ObservableSubscription using SymbolWrapper::isStringSymbol(), SymbolWrapper::isWildcardSymbol(),
     * SymbolWrapper::isIndexedEventSubscriptionSymbol(), SymbolWrapper::isTimeSeriesSubscriptionSymbol(),
     * SymbolWrapper::isCandleSymbol().
     *
     * <p> The set of subscribed symbols contains WildcardSymbol::ALL if and
     * only if there is a subscription to this wildcard symbol.
     *
     * <p> If DXFeedTimeSeriesSubscription is used to subscribe to time-service of the events of this type, then
     * instances of TimeSeriesSubscriptionSymbol class represent the corresponding subscription item.
     *
     * <p> The resulting observable subscription can generate repeated ObservableSubscriptionChangeListener::onSymbolsAdded_ notifications to
     * its listeners for the same symbols without the corresponding ObservableSubscriptionChangeListener::onSymbolsRemoved_
     * notifications in between them. It happens when subscription disappears, cached data is lost, and subscription
     * reappears again. On each ObservableSubscriptionChangeListener::onSymbolsAdded_
     * notification data provider shall @ref DXPublisher::publishEvents() "publish" the most recent events for
     * the corresponding symbols.
     *
     * @param eventType The event type.
     * @return Observable subscription for the specified event type.
     */
    std::shared_ptr<ObservableSubscription> getSubscription(const EventTypeEnum &eventType);

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()