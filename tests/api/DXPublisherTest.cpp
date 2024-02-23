// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include <doctest.h>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

//Segfaults appear sporadically on Linux and MacOS.
//#ifdef _WIN32

TEST_CASE("DXPublisher::publishEvents") {
    System::setProperty("dxendpoint.eventTime", "true");
    System::setProperty("dxscheme.nanoTime", "true");

    auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    auto feed = endpoint->getFeed();
    auto publisher = endpoint->getPublisher();
    auto sub = feed->createSubscription({Quote::TYPE});
    auto quote = std::make_shared<Quote>("AAPL");

    quote->setEventTime(1692975409000);
    quote->setTimeNanoPart(123);
    quote->setSequence(123);
    quote->setBidTime(1692975409000);
    quote->setBidExchangeCode('B');
    quote->setBidPrice(176.08);
    quote->setBidSize(1.0);
    quote->setAskTime(1692975409000);
    quote->setAskExchangeCode('A');
    quote->setAskPrice(176.16);
    quote->setAskSize(2.0);

    std::vector<std::shared_ptr<EventType>> result{};
    std::mutex mtx{};

    sub->addEventListener([&mtx, &result](const auto &events) {
        for (const auto &e : events) {
            if (const auto &q = e->template sharedAs<Quote>(); q) {
                std::cout << q << std::endl;

                std::lock_guard lock{mtx};
                result.push_back(std::make_shared<Quote>(*q));
            }
        }
    });

    sub->addSymbols("AAPL");
    publisher->publishEvents(quote);
    publisher->publishEvents({quote, quote});

    std::vector<EventType::Ptr> events{};
    events.push_back(quote);
    events.push_back(quote);

    publisher->publishEvents(events);

    std::this_thread::sleep_for(1000ms);

    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();

    std::lock_guard lock{mtx};

    REQUIRE_EQ(result.size(), 1);
    REQUIRE_EQ(quote->toString(), result.back()->toString());
}

//#endif