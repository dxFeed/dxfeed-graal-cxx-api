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