// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

namespace {

using Quotes = std::vector<std::shared_ptr<Quote>>;

// getLastEvents returns a reference to a named collection and a temporary collection by value.
static_assert(std::is_same_v<decltype(std::declval<DXFeed &>().getLastEvents(std::declval<Quotes &>())), Quotes &>);
static_assert(
    std::is_same_v<decltype(std::declval<DXFeed &>().getLastEvents(std::declval<const Quotes &>())), const Quotes &>);
static_assert(std::is_same_v<decltype(std::declval<DXFeed &>().getLastEvents(std::declval<Quotes>())), Quotes>);

// getLastEvents accepts only collections of shared ptrs of lasting events (the LastingEventCollection concept).
template <typename Collection>
concept CanGetLastEvents =
    requires(DXFeed &feed, Collection &&events) { feed.getLastEvents(std::forward<Collection>(events)); };

static_assert(LastingEventCollection<Quotes>);
static_assert(LastingEventCollection<const std::list<std::shared_ptr<Trade>> &>);
static_assert(CanGetLastEvents<Quotes &>);
static_assert(CanGetLastEvents<Quotes>);
static_assert(!CanGetLastEvents<std::vector<int> &>);
static_assert(!CanGetLastEvents<std::vector<Quote> &>);                        // not shared ptrs
static_assert(!CanGetLastEvents<std::vector<std::shared_ptr<TimeAndSale>> &>); // TimeAndSale is not a LastingEvent

// A LOCAL_HUB with a Quote subscription to AAPL and the published quote AAPL (bid 10.5); IBM is not subscribed.
struct LastEventFixture : test::LocalHubFixture {
    std::shared_ptr<DXFeedSubscription> subscription;

    LastEventFixture() : subscription(feed->createSubscription(Quote::TYPE)) {
        subscription->addSymbols("AAPL");

        auto quote = std::make_shared<Quote>("AAPL");

        quote->setBidPrice(10.5);
        publishAndProcess(std::vector{quote});
    }
};

} // namespace

TEST_CASE_FIXTURE(LastEventFixture, "DXFeed::getLastEvent fills the event with the last event of a subscribed symbol") {
    const auto aapl = std::make_shared<Quote>("AAPL");
    const auto ibm = std::make_shared<Quote>("IBM");

    CHECK(feed->getLastEvent(aapl) == aapl);
    CHECK(aapl->getBidPrice() == 10.5);
    CHECK(feed->getLastEvent(ibm) == ibm);
    CHECK(std::isnan(ibm->getBidPrice()));
}

TEST_CASE_FIXTURE(LastEventFixture, "DXFeed::getLastEvents fills a named collection and returns a reference to it") {
    Quotes quotes{std::make_shared<Quote>("AAPL"), std::make_shared<Quote>("IBM")};

    auto &result = feed->getLastEvents(quotes);

    CHECK(&result == &quotes);
    CHECK(quotes[0]->getBidPrice() == 10.5);
    CHECK(std::isnan(quotes[1]->getBidPrice()));

    const Quotes constQuotes{std::make_shared<Quote>("AAPL")};

    CHECK(&feed->getLastEvents(constQuotes) == &constQuotes);
    CHECK(constQuotes[0]->getBidPrice() == 10.5);
}

TEST_CASE_FIXTURE(LastEventFixture, "DXFeed::getLastEvents returns a temporary collection by value") {
    const auto aapl = std::make_shared<Quote>("AAPL");
    std::size_t count = 0;

    // The temporary vector is moved into the result, which lives until the end of the loop (with a reference to the
    // temporary this would be a use after scope in C++20).
    for (const auto &quote : feed->getLastEvents(Quotes{aapl})) {
        CHECK(quote == aapl);
        CHECK(quote->getBidPrice() == 10.5);
        count++;
    }

    CHECK(count == 1);
}

TEST_CASE_FIXTURE(LastEventFixture, "DXFeed::getLastEvents accepts other collections of lasting events") {
    std::list<std::shared_ptr<Quote>> quotes{std::make_shared<Quote>("AAPL")};

    CHECK(&feed->getLastEvents(quotes) == &quotes);
    CHECK(quotes.front()->getBidPrice() == 10.5);
}

TEST_CASE("DXFeed::getIndexedEventsIfSubscribed should return an empty vector when not subscribed to any events") {
    const auto events = DXFeed::getInstance()->getIndexedEventsIfSubscribed<Order>("AAPL", OrderSource::ntv);

    REQUIRE(events.empty());
}

struct DXFeedTestFixture {
    const char *symbol = "TEST";
    DXEndpoint::Ptr endpoint{};
    DXFeed::Ptr feed{};
    DXPublisher::Ptr pub{};
    std::shared_ptr<InPlaceExecutor> executor{};
    std::vector<std::shared_ptr<EventType>> publishedEvents{};

    DXFeedTestFixture() {
        executor = InPlaceExecutor::create();
        endpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::LOCAL_HUB)
                       ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                       ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")
                       ->withProperty(DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true")
                       ->build();
        endpoint->executor(executor);
        feed = endpoint->getFeed();
        pub = endpoint->getPublisher();
    }

    std::shared_ptr<Order> createOrder(std::int64_t index, const Side &side, double price, double size,
                                       std::int32_t eventFlags) {
        return std::make_shared<Order>(symbol)
            ->withIndex(index)
            .withOrderSide(side)
            .withPrice(price)
            .withSize(size)
            .withEventFlags(eventFlags)
            .sharedAs<Order>();
    }

    std::shared_ptr<Candle> createCandle(std::int64_t time) const {
        return std::make_shared<Candle>(CandleSymbol::valueOf(symbol))
            ->withTime(time)
            .withClose(42.0)
            .withVolume(1000)
            .sharedAs<Candle>();
    }

    void publish(const auto &event) {
        publishedEvents.push_back(event);
    }

    void publishMany(const auto &events) {
        publishedEvents.insert(publishedEvents.end(), std::begin(events), std::end(events));
    }

    void process() {
        pub->publishEvents(publishedEvents);
        executor->processAllPendingTasks();
        publishedEvents.clear();
    }

    void publishAndProcess(const auto &event) {
        publish(event);
        process();
    }

    void publishAndProcessMany(const auto &events) {
        publishMany(events);
        process();
    }

    ~DXFeedTestFixture() {
        endpoint->awaitProcessed();
        endpoint->close();
    }
};

TEST_CASE_FIXTURE(DXFeedTestFixture, "DXFeed::getIndexedEventsIfSubscribed should return events") {
    const auto o = createOrder(
        0, Side::BUY, 1, 1, static_cast<std::int32_t>((EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask()));
    const auto sub = feed->createSubscription(Order::TYPE);

    sub->addSymbols(symbol);
    publishAndProcess(o);

    const auto events = feed->getIndexedEventsIfSubscribed<Order>(symbol, OrderSource::DEFAULT);

    REQUIRE(events.size() == 1);
}

TEST_CASE("DXFeed::getTimeSeriesIfSubscribed should return an empty vector when not subscribed to any events") {
    const auto fromTime = std::chrono::milliseconds(now()) - std::chrono::days(1);
    constexpr auto toTime = std::chrono::milliseconds(std::numeric_limits<std::int64_t>::max());
    const auto events =
        DXFeed::getInstance()->getTimeSeriesIfSubscribed<Candle>("AAPL", fromTime.count(), toTime.count());

    REQUIRE(events.empty());
}

TEST_CASE_FIXTURE(DXFeedTestFixture, "DXFeed::getTimeSeriesIfSubscribed should return events") {
    const auto fromTime = std::chrono::milliseconds(now()) - std::chrono::days(1);
    const auto c = createCandle(fromTime.count());
    const auto sub = feed->createSubscription(Candle::TYPE);

    sub->addSymbols(TimeSeriesSubscriptionSymbol(symbol, fromTime.count()));

    publishAndProcess(c);

    const auto events = feed->getTimeSeriesIfSubscribed<Candle>(symbol, fromTime.count());

    REQUIRE(events.size() == 1);
    REQUIRE(events[0]->getTime() == fromTime.count());
    REQUIRE(events[0]->getClose() == doctest::Approx(c->getClose()));
    REQUIRE(events[0]->getVolume() == doctest::Approx(c->getVolume()));
}