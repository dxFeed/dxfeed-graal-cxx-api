// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

using namespace dxfcpp;
using namespace std::literals;

// The expected strings are the output of Java `TimePeriodInfo.toString()` (dxFeed Java API 3.356).
TEST_CASE("TimePeriodInfo::toString() is the same as in Java") {
    REQUIRE(TimePeriodInfo::UNKNOWN.toString() == R"({"min":-1,"max":-1})");
    REQUIRE(TimePeriodInfo::EMPTY.toString() == R"({"min":0,"max":0})");
    REQUIRE(TimePeriodInfo::valueOf(1500, 2000).toString() == R"({"min":1.5,"max":2})");
    REQUIRE(TimePeriodInfo::valueOf(1, 1).toString() == R"({"min":0.001,"max":0.001})");
    REQUIRE(TimePeriodInfo::valueOf(0, 250).toString() == R"({"min":0,"max":0.25})");
    REQUIRE(TimePeriodInfo::valueOf(123456, 86400000).toString() == R"({"min":123.456,"max":86400})");
    REQUIRE(TimePeriodInfo::valueOf(5, 5000).toString() == R"({"min":0.005,"max":5})");
    REQUIRE(TimePeriodInfo::valueOf(1234567890123, 9999999999999).toString() ==
            R"({"min":1234567890.123,"max":9999999999.999})");
    REQUIRE(TimePeriodInfo::valueOf(9999999999999, 10000000000000).toString() ==
            R"({"min":9999999999.999,"max":1E10})");
    REQUIRE(TimePeriodInfo::valueOf(10000000000001, 12345678901234567).toString() ==
            R"({"min":1.0000000000001E10,"max":1.2345678901234568E13})");
    REQUIRE(TimePeriodInfo::valueOf(99999999999999, 100000000000000).toString() ==
            R"({"min":9.9999999999999E10,"max":1E11})");

    constexpr auto maxValue = std::numeric_limits<std::int64_t>::max();

    REQUIRE(TimePeriodInfo::valueOf(maxValue, maxValue).toString() ==
            R"({"min":9.223372036854776E15,"max":9.223372036854776E15})");
}

TEST_CASE("TimePeriodInfo values") {
    REQUIRE(TimePeriodInfo::UNKNOWN.isUnknown());
    REQUIRE(TimePeriodInfo::UNKNOWN.getMin() == -1000);
    REQUIRE(TimePeriodInfo::UNKNOWN.getMax() == -1000);
    REQUIRE_FALSE(TimePeriodInfo::EMPTY.isUnknown());
    REQUIRE(TimePeriodInfo::EMPTY.getMin() == 0);
    REQUIRE(TimePeriodInfo::EMPTY.getMax() == 0);
    REQUIRE(TimePeriodInfo::valueOf(0, 0) == TimePeriodInfo::EMPTY);

    const auto info = TimePeriodInfo::valueOf(5, 5000);

    REQUIRE(info.getMin() == 5);
    REQUIRE(info.getMax() == 5000);
    REQUIRE_FALSE(info.isUnknown());
    REQUIRE(info == TimePeriodInfo::valueOf(5, 5000));
    REQUIRE(info != TimePeriodInfo::valueOf(5, 5001));
    REQUIRE(info != TimePeriodInfo::UNKNOWN);

    REQUIRE_THROWS_WITH_AS(TimePeriodInfo::valueOf(-1, 0),
                           doctest::Contains("min and max must be non-negative, got: (-1,0)"),
                           InvalidArgumentException);
    REQUIRE_THROWS_WITH_AS(TimePeriodInfo::valueOf(0, -1),
                           doctest::Contains("min and max must be non-negative, got: (0,-1)"),
                           InvalidArgumentException);
    REQUIRE_THROWS_WITH_AS(TimePeriodInfo::valueOf(2, 1),
                           doctest::Contains("min (2) must be less than or equal to max (1)"),
                           InvalidArgumentException);
}

TEST_CASE("DXEndpoint: the requested aggregation period can be set, read and cleared") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    REQUIRE_FALSE(endpoint->getRequestedAggregationPeriod().has_value());
    REQUIRE(endpoint->getAggregationPeriodInfo().isUnknown());

    endpoint->setRequestedAggregationPeriod(TimePeriod::valueOf(5000));
    REQUIRE(endpoint->getRequestedAggregationPeriod().has_value());
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 5000);

    // Zero requests real-time delivery: it is a requested period, not a cleared one.
    endpoint->setRequestedAggregationPeriod(TimePeriod::ZERO);
    REQUIRE(endpoint->getRequestedAggregationPeriod().has_value());
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 0);

    endpoint->setRequestedAggregationPeriod(std::nullopt);
    REQUIRE_FALSE(endpoint->getRequestedAggregationPeriod().has_value());

    endpoint->setRequestedAggregationPeriod(250ms);
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 250);

    endpoint->setRequestedAggregationPeriod(std::int64_t{86400000});
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 86400000);

    // Over 24 hours the period is rejected and the previous one stays.
    REQUIRE_THROWS_WITH_AS(endpoint->setRequestedAggregationPeriod(std::int64_t{86400001}),
                           doctest::Contains("Aggregation period is too large"), JavaException);
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 86400000);

    REQUIRE_THROWS_AS(endpoint->setRequestedAggregationPeriod(-5ms), JavaException);

    endpoint->close();
}

TEST_CASE("DXEndpoint: the requested aggregation period can be set with a property") {
    const auto endpoint = DXEndpoint::newBuilder()
                              ->withRole(DXEndpoint::Role::FEED)
                              ->withProperty(DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY, "1.5s")
                              ->build();

    REQUIRE(endpoint->getRequestedAggregationPeriod().has_value());
    REQUIRE(endpoint->getRequestedAggregationPeriod()->getTime() == 1500);

    endpoint->close();
}

TEST_CASE("DXFeedSubscription: a new subscription has no aggregation period info and no dropped events") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

    REQUIRE(subscription->getAggregationPeriodInfo().isUnknown());
    REQUIRE(subscription->getTotalDroppedEvents() == 0);

    subscription->close();
    endpoint->close();
}

// A FEED requests an aggregation period from a loopback PUBLISHER; both report it once connected.
TEST_CASE("DXEndpoint: the aggregation period info is reported over a connection") {
    const auto port = std::to_string(test::findFreeLoopbackPort());
    const auto publisherEndpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto feedEndpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    feedEndpoint->setRequestedAggregationPeriod(1000ms);
    REQUIRE(feedEndpoint->getAggregationPeriodInfo().isUnknown());

    publisherEndpoint->connect(":" + port + "[bindAddr=127.0.0.1]");
    REQUIRE(test::waitUntil([&] {
        return publisherEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));
    feedEndpoint->connect("127.0.0.1:" + port);
    REQUIRE(test::waitUntil([&] {
        return feedEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));

    const auto expected = TimePeriodInfo::valueOf(1000, 1000);

    REQUIRE(test::waitUntil([&] {
        return feedEndpoint->getAggregationPeriodInfo() == expected &&
               publisherEndpoint->getAggregationPeriodInfo() == expected;
    }));

    feedEndpoint->close();
    publisherEndpoint->close();
}

namespace {
struct AggregationPeriodInfos {
    std::string subscription;
    std::string endpoint;
};

// Publishes quotes from a loopback PUBLISHER until a FEED subscription receives them, then returns the aggregation
// period info of the subscription and of the FEED endpoint.
AggregationPeriodInfos infosAfterEvents(const std::optional<std::string> &aggregationPeriodProperty,
                                        std::optional<std::chrono::milliseconds> requestedPeriod,
                                        std::optional<std::chrono::milliseconds> subscriptionPeriod) {
    const auto port = std::to_string(test::findFreeLoopbackPort());
    const auto publisherEndpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto builder = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED);

    if (aggregationPeriodProperty) {
        builder->withProperty(DXEndpoint::DXFEED_AGGREGATION_PERIOD_PROPERTY, *aggregationPeriodProperty);
    }

    const auto feedEndpoint = builder->build();

    if (requestedPeriod) {
        feedEndpoint->setRequestedAggregationPeriod(*requestedPeriod);
    }

    const auto subscription = feedEndpoint->getFeed()->createSubscription(Quote::TYPE);

    if (subscriptionPeriod) {
        subscription->setAggregationPeriod(*subscriptionPeriod);
    }

    std::atomic<std::size_t> received{};

    subscription->addEventListener([&received](const auto &events) {
        received += events.size();
    });
    subscription->addSymbols("AAPL");
    REQUIRE(subscription->getAggregationPeriodInfo().isUnknown());

    publisherEndpoint->connect(":" + port + "[bindAddr=127.0.0.1]");
    REQUIRE(test::waitUntil([&] {
        return publisherEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));
    feedEndpoint->connect("127.0.0.1:" + port);
    REQUIRE(test::waitUntil([&] {
        return feedEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));

    double price = 1.0;

    // Each poll publishes one more quote: the first ones may be sent before the subscription reaches the publisher.
    REQUIRE(test::waitUntil(
        [&] {
            const auto quote = std::make_shared<Quote>("AAPL");

            quote->setBidPrice(price++);
            publisherEndpoint->getPublisher()->publishEvents(quote);

            return received > 0 && !subscription->getAggregationPeriodInfo().isUnknown() &&
                   !feedEndpoint->getAggregationPeriodInfo().isUnknown();
        },
        10s, 50ms));

    AggregationPeriodInfos result{subscription->getAggregationPeriodInfo().toString(),
                                  feedEndpoint->getAggregationPeriodInfo().toString()};

    subscription->close();
    feedEndpoint->close();
    publisherEndpoint->close();

    return result;
}
} // namespace

// The subscription reports the larger of `dxfeed.aggregationPeriod` and its own aggregation period once events are
// delivered; the period requested from the server is reported by the endpoint only. The expected strings are the
// output of the same scenarios in the dxFeed Java API 3.356.
TEST_CASE("DXFeedSubscription: the aggregation period info after events") {
    SUBCASE("no aggregation") {
        const auto infos = infosAfterEvents(std::nullopt, std::nullopt, std::nullopt);

        CHECK(infos.subscription == R"({"min":0,"max":0})");
        CHECK(infos.endpoint == R"({"min":0,"max":0})");
    }

    SUBCASE("the aggregation period of the subscription") {
        const auto infos = infosAfterEvents(std::nullopt, std::nullopt, 200ms);

        CHECK(infos.subscription == R"({"min":0.2,"max":0.2})");
        CHECK(infos.endpoint == R"({"min":0,"max":0})");
    }

    SUBCASE("the requested aggregation period does not apply to the subscription") {
        const auto infos = infosAfterEvents(std::nullopt, 1000ms, 200ms);

        CHECK(infos.subscription == R"({"min":0.2,"max":0.2})");
        CHECK(infos.endpoint == R"({"min":1,"max":1})");
    }

    SUBCASE("dxfeed.aggregationPeriod is larger than the period of the subscription") {
        const auto infos = infosAfterEvents("0.5s", std::nullopt, 200ms);

        CHECK(infos.subscription == R"({"min":0.5,"max":0.5})");
        CHECK(infos.endpoint == R"({"min":0,"max":0})");
    }

    SUBCASE("dxfeed.aggregationPeriod is smaller than the period of the subscription") {
        const auto infos = infosAfterEvents("0.1s", std::nullopt, 300ms);

        CHECK(infos.subscription == R"({"min":0.3,"max":0.3})");
        CHECK(infos.endpoint == R"({"min":0,"max":0})");
    }
}
