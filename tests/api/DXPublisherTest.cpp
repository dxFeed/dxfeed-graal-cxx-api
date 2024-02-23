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

TEST_CASE("DXPublisher::publishEvents") {
    try {
        System::setProperty("dxendpoint.eventTime", "true");
        System::setProperty("dxscheme.nanoTime", "true");

        auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
        auto feed = endpoint->getFeed();
        auto publisher = endpoint->getPublisher();
        auto sub = feed->createSubscription({Quote::TYPE});
        auto quote = std::make_shared<Quote>("AAPL")
                         ->withEventTimeShared(1692975409000)
                         ->withTimeNanoPartShared(123)
                         ->withSequenceShared(123)
                         ->withBidTimeShared(1692975409000)
                         ->withBidExchangeCodeShared('B')
                         ->withBidPriceShared(176.08)
                         ->withBidSizeShared(1.0)
                         ->withAskTimeShared(1692975409000)
                         ->withAskExchangeCodeShared('A')
                         ->withAskPriceShared(176.16)
                         ->withAskSizeShared(2.0);

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

        std::this_thread::sleep_for(100ms);

        endpoint->awaitProcessed();
        endpoint->closeAndAwaitTermination();

        std::lock_guard lock{mtx};

        REQUIRE_EQ(result.size(), 1);
        REQUIRE_EQ(quote->toString(), result.back()->toString());
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
