// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

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
            ->withTime(time).withClose(42.0).withVolume(1000)
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