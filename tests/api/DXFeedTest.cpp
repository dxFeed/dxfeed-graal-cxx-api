// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("DXFeed::getIndexedEventsIfSubscribed should return an empty vector when not subscribed to any events") {
    auto events = DXFeed::getInstance()->getIndexedEventsIfSubscribed<Order>("AAPL", OrderSource::ntv);

    REQUIRE(events.empty());
}

struct DXFeedTestFixture {
    const char *symbol = "TEST";
    DXEndpoint::Ptr endpoint{};
    DXFeed::Ptr feed{};
    DXPublisher::Ptr pub{};

    DXFeedTestFixture() {
        endpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::LOCAL_HUB)
                       ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                       ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")
                       ->withProperty(DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true")
                       ->build();
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

    ~DXFeedTestFixture() {
        endpoint->awaitProcessed();
        endpoint->close();
    }
};

TEST_CASE_FIXTURE(DXFeedTestFixture, "DXFeed::getIndexedEventsIfSubscribed should return events") {
    auto o = createOrder(0, Side::BUY, 1, 1,
                         static_cast<std::int32_t>((EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask()));

    auto sub = feed->createSubscription(Order::TYPE);
    sub->addSymbols(symbol);

    pub->publishEvents(o);
    auto events = feed->getIndexedEventsIfSubscribed<Order>(symbol, OrderSource::DEFAULT);

    REQUIRE(events.size() == 1);
}