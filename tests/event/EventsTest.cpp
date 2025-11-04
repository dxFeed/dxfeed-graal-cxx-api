// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("getLastEventsPromises test") {
    // Disable QD logging.
    Logging::init();

    const auto endpoint = DXEndpoint::create()->connect("demo.dxfeed.com:7300");
    const auto promises = endpoint->getFeed()->getLastEventsPromises<Quote>({"AAPL&Q", "IBM&Q"});

    // combine the list of promises into one with Promises utility method and wait
    Promises::allOf(*promises)->awaitWithoutException(std::chrono::seconds(5s));

    // now iterate the promises to retrieve results
    for (const auto &promise : *promises) {
        auto r = promise.getResult();

        std::cout << r << std::endl;
    }

    std::vector<std::shared_ptr<Promise<std::shared_ptr<Quote>>>> promises2{};

    for (auto s : {"AAPL&Q", "IBM&Q"}) {
        promises2.push_back(endpoint->getFeed()->getLastEventPromise<Quote>(s));
    }

    // ignore timeout and continue to print retrieved quotes even on timeout
    auto ok = Promises::allOf(promises2)->awaitWithoutException(std::chrono::seconds(5));
    ignoreUnused(ok);

    for (const auto &promise : promises2) {
        auto quote = promise->getResult();

        std::cout << quote << std::endl;
    }
}

TEST_CASE("The Candle::assign method must set all fields") {
    auto c1 = std::make_shared<Candle>(CandleSymbol::valueOf("AAPL&Q"));
    c1->setEventTime(100);

    auto c2 = std::make_shared<Candle>(CandleSymbol::valueOf("IBM"));

    c2->setEventTime(1200);

    // index
    c2->setTime(222);
    c2->setSequence(23);

    c2->setCount(5);

    c2->setOpen(50.1);
    c2->setHigh(100.2);
    c2->setLow(40.3);
    c2->setClose(34.4);

    c2->setVolume(1000.34);
    c2->setVWAP(34.22);
    c2->setBidVolume(1234.33);
    c2->setAskVolume(14.44);
    c2->setImpVolatility(200);
    c2->setOpenInterest(200);

    auto c3 = std::make_shared<Candle>();

    c3->assign(c1);
    c1->assign(c2);

    REQUIRE(c1->toString() == c2->toString());
    REQUIRE(c1->toString() != c3->toString());
}