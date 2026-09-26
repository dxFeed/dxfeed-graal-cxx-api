// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

namespace {

struct DataIntegrityTestFixture : test::LocalHubFixture {
    DataIntegrityTestFixture()
        : LocalHubFixture({{DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true"},
                           {DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true"},
                           {DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true"}}) {
    }
};

std::shared_ptr<TimeAndSale> createTimeAndSale(const std::string &symbol, std::int64_t time) {
    auto tns = std::make_shared<TimeAndSale>(symbol);

    tns->setTime(time);

    return tns;
}

} // namespace

TEST_CASE_FIXTURE(DataIntegrityTestFixture, "dxFeed :: Test attach & detach sub") {
    std::vector<std::string> receivedAAA{};
    std::vector<std::string> receivedBBB{};

    const auto tnsSubAAA = DXFeedSubscription::create({TimeAndSale::TYPE});
    const auto tnsSubBBB = DXFeedSubscription::create({TimeAndSale::TYPE});

    tnsSubAAA->addSymbols("AAA");
    tnsSubBBB->addSymbols("BBB");

    tnsSubAAA->addEventListener<TimeAndSale>([&receivedAAA](const auto &timeAndSales) {
        for (auto &&tns : timeAndSales) {
            receivedAAA.push_back(tns->getEventSymbol());
        }
    });

    tnsSubBBB->addEventListener<TimeAndSale>([&receivedBBB](const auto &timeAndSales) {
        for (auto &&tns : timeAndSales) {
            receivedBBB.push_back(tns->getEventSymbol());
        }
    });

    auto time = 1000;
    const auto publishBoth = [&] {
        publishAndProcess(std::vector<std::shared_ptr<TimeAndSale>>{createTimeAndSale("AAA", time),
                                                                    createTimeAndSale("BBB", time)});
        time += 1000;
    };

    publishBoth(); // nothing is attached yet
    REQUIRE(receivedAAA.empty());
    REQUIRE(receivedBBB.empty());

    feed->attachSubscription(tnsSubAAA);
    feed->attachSubscription(tnsSubBBB);
    publishBoth();
    REQUIRE(receivedAAA == std::vector<std::string>{"AAA"});
    REQUIRE(receivedBBB == std::vector<std::string>{"BBB"});

    feed->detachSubscription(tnsSubAAA);
    publishBoth();
    REQUIRE(receivedAAA.size() == 1); // detached
    REQUIRE(receivedBBB == std::vector<std::string>{"BBB", "BBB"});

    feed->detachSubscription(tnsSubBBB);
    publishBoth();
    REQUIRE(receivedAAA.size() == 1);
    REQUIRE(receivedBBB.size() == 2);
}

TEST_CASE_FIXTURE(DataIntegrityTestFixture, "dxFeed :: Test TextMessage") {
    std::vector<std::string> receivedTexts{};
    const auto sub = DXFeedSubscription::create({TextMessage::TYPE});

    sub->addSymbols("TOKEN");
    sub->addEventListener<TextMessage>([&receivedTexts](const auto &textMessages) {
        for (auto &&t : textMessages) {
            REQUIRE(t->getEventSymbol() == "TOKEN");
            receivedTexts.push_back(t->getText());
        }
    });

    feed->attachSubscription(sub);

    for (auto i = 0; i < 3; i++) {
        const auto t = std::make_shared<TextMessage>("TOKEN");

        t->setTime(1000 * (i + 1));
        t->setText(std::to_string(i));
        publishAndProcess(t);
    }

    REQUIRE(receivedTexts == std::vector<std::string>{"0", "1", "2"});

    feed->detachSubscription(sub);

    const auto t = std::make_shared<TextMessage>("TOKEN");

    t->setText("after detach");
    publishAndProcess(t);
    REQUIRE(receivedTexts.size() == 3);
}

// A PUBLISHER listens only on the loopback interface (`bindAddr`, Graal SDK v3.5.0+), a FEED connects to it over TCP.
TEST_CASE("Events are delivered from a loopback PUBLISHER to a FEED over TCP") {
    const auto port = std::to_string(test::findFreeLoopbackPort());
    const auto publisherEndpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto feedEndpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    publisherEndpoint->connect(":" + port + "[bindAddr=127.0.0.1]");
    feedEndpoint->connect("127.0.0.1:" + port);
    REQUIRE(test::waitUntil([&] {
        return feedEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));

    std::mutex mutex{};
    std::vector<double> bidPrices{};
    const auto subscription = feedEndpoint->getFeed()->createSubscription(Quote::TYPE);

    subscription->addEventListener<Quote>([&](const auto &quotes) {
        std::lock_guard lock{mutex};

        for (const auto &quote : quotes) {
            bidPrices.push_back(quote->getBidPrice());
        }
    });
    subscription->addSymbols("TCP-TEST");

    // The subscription reaches the publisher asynchronously, and a quote published before that is not delivered.
    const auto quote = std::make_shared<Quote>("TCP-TEST");

    quote->setBidPrice(12.5);
    REQUIRE(test::waitUntil(
        [&] {
            publisherEndpoint->getPublisher()->publishEvents(quote);

            std::lock_guard lock{mutex};

            return !bidPrices.empty();
        },
        10s, 100ms));

    {
        std::lock_guard lock{mutex};

        REQUIRE(bidPrices.front() == 12.5);
    }

    subscription->close();
    feedEndpoint->close();
    publisherEndpoint->close();
}
