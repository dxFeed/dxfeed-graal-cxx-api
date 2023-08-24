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
 *
 * <h3>Sample usage</h3>
 *
 * This section gives sample usage scenarios.
 *
 * <h4>Default singleton instance</h4>
 *
 * There is a singleton instance of the feed that is returned by ::getInstance() method.
 * It is created on the first use with default configuration properties that are explained in detail in
 * documentation for DXEndpoint class in the "Default properties" section.
 *
 * <p>In particular,
 * you can provide a default address to connect and credentials  using
 * "@ref DXEndpoint::DXFEED_ADDRESS_PROPERTY "dxfeed.address"",
 * "@ref DXEndpoint::DXFEED_USER_PROPERTY "dxfeed.user"", and
 * "{@link DXEndpoint#DXFEED_PASSWORD_PROPERTY dxfeed.password}"
 * system properties or by putting them into
 * "{@link DXEndpoint#DXFEED_PROPERTIES_PROPERTY dxfeed.properties}"
 * file on JVM classpath. dxFeed API samples come with a ready-to-use "<b>dxfeed.properties</b>"
 * file that contains an address of dxFeed demo feed at "<b>demo.dxfeed.com:7300</b>" and
 * demo access credentials.
 *
 * <h4>Subscribe for single event type</h4>
 *
 * The following code creates listener that prints mid price for each quote
 * and subscribes for quotes on SPDR S&amp;P 500 ETF symbol:
 * <pre><tt>
 * {@link DXFeedSubscription DXFeedSubscription}&lt;{@link Quote Quote}&gt; sub = {@link DXFeed DXFeed}.{@link #getInstance() getInstance}().{@link #createSubscription(Class) createSubscription}({@link Quote Quote.class});
 * sub.{@link DXFeedSubscription#addEventListener addEventListener}(new {@link DXFeedEventListener DXFeedEventListener}&lt;{@link Quote Quote}&gt;() {
 *     public void eventsReceived({@link List List}&lt;{@link Quote Quote}&gt; quotes) {
 *         for ({@link Quote Quote} quote : quotes)
 *             System.out.println("Mid = " + (quote.{@link Quote#getBidPrice getBidPrice}() + quote.{@link Quote#getAskPrice getAskPrice}()) / 2);
 *     }
 * });
 * sub.{@link DXFeedSubscription#addSymbols(Object...) addSymbols}("SPY");</tt></pre>
 *
 * Note, that order of calls is important here. By attaching listeners first and then setting
 * subscription we ensure that the current quote gets received by the listener. See
 * {@link DXFeedSubscription#addSymbols(Object...) DXFeedSubscription.addSymbols} for details.
 * If a set of symbols is changed first, then {@link DXFeedSubscription#addEventListener(DXFeedEventListener) sub.addEventListener}
 * raises an {@link IllegalStateException} to protected from hard-to-catch bugs with potentially missed events.
 *
 * <h4>Subscribe for multiple event types</h4>
 *
 * The following code creates listener that prints each received event and
 * subscribes for quotes and trades on SPDR S&amp;P 500 ETF symbol:
 * <pre><tt>
 * {@link DXFeedSubscription DXFeedSubscription}&lt;{@link MarketEvent MarketEvent}&gt; sub = {@link DXFeed DXFeed}.{@link #getInstance() getInstance}().&lt;{@link MarketEvent MarketEvent}&gt;{@link #createSubscription(Class[]) createSubscription}({@link Quote Quote.class}, {@link Trade Trade.class});
 * sub.{@link DXFeedSubscription#addEventListener addEventListener}(new {@link DXFeedEventListener DXFeedEventListener}&lt;{@link MarketEvent MarketEvent}&gt;() {
 *     public void eventsReceived({@link List List}&lt;{@link MarketEvent MarketEvent}&gt; events) {
 *         for ({@link MarketEvent MarketEvent} event : events)
 *             System.out.println(event);
 *     }
 * });
 * sub.{@link DXFeedSubscription#addSymbols(Object...) addSymbols}("SPY");</tt></pre>
 *
 * <h4>Subscribe for event and query periodically its last value</h4>
 *
 * The following code subscribes for trades on SPDR S&amp;P 500 ETF symbol and
 * prints last trade every second.
 *
 * <pre><tt>
 * {@link DXFeedSubscription DXFeedSubscription}&lt;{@link Trade Trade}&gt; sub = {@link DXFeed DXFeed}.{@link #getInstance() getInstance}().{@link #createSubscription(Class) createSubscription}({@link Trade Trade.class});
 * sub.{@link DXFeedSubscription#addSymbols(Object...) addSymbols}("SPY");
 * while (true) {
 *     System.out.println(feed.{@link #getLastEvent getLastEvent}(new Trade("SPY")));
 *     Thread.sleep(1000);
 * }</tt></pre>
 *
 * <h3>Threads and locks</h3>
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 *
 * <h3>Implementation details</h3>
 *
 * dxFeed API is implemented on top of QDS. dxFeed API classes itself are in "<b>dxfeed-api.jar</b>", but
 * their implementation is in "<b>qds.jar</b>". You need have "<b>qds.jar</b>" in your classpath
 * in order to use dxFeed API.
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
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription(dxfcpp::Quote::TYPE);
     * ```
     *
     * @param eventType The type of event
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(const EventTypeEnum &eventType) noexcept;

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
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
     * Example:
     * ```cpp
     * auto sub = dxfcpp::DXFeed::getInstance()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});
     * ```
     *
     * @param eventTypes The initializer list of event types
     * @return The new subscription
     */
    std::shared_ptr<DXFeedSubscription> createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept;

    /**
     * Creates new subscription for multiple event types that is <i>attached</i> to this feed.
     * This method creates new DXFeedSubscription and invokes @link ::attachSubscription.
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