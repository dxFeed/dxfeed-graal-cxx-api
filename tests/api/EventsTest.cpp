// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include <doctest.h>
#include <fmt/format.h>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

struct OrderConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
};

TEST_CASE("Order::setTime() should change TimeSequence") {

    auto o = Order("AAPL").withSequence(123);

    auto oldTimeSequence = o.getTimeSequence();

    std::cout << "OTS:" << oldTimeSequence << std::endl;

    std::int64_t time = 1'701'703'226'500LL;

    std::cout << "T: " << time << std::endl;
    fmt::println("{:0B}", time);
    fmt::println("{:0B}", static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)));
    fmt::println("{:0B}", dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                      OrderConstants::SECONDS_SHIFT));
    fmt::println("{:0B}", static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)));
    fmt::println("{:0B}", dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                      OrderConstants::MILLISECONDS_SHIFT));
    fmt::println("{:0B}",
                 dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                          OrderConstants::SECONDS_SHIFT),
                              dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                          OrderConstants::MILLISECONDS_SHIFT)));
    fmt::println(
        "{:0B}",
        dxfcpp::orOp(dxfcpp::orOp(sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                      OrderConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              OrderConstants::MILLISECONDS_SHIFT)),
                     o.getSequence()));

    std::cout << "S: " << static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)) << std::endl;
    std::cout << "MS: " << static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)) << std::endl;

    o.setTime(time);

    auto newTimeSequence = o.getTimeSequence();

    std::cout << "NTS: " << newTimeSequence << std::endl;

    auto expectedTime = dxfcpp::sar(newTimeSequence, OrderConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(newTimeSequence, OrderConstants::MILLISECONDS_SHIFT),
                                      OrderConstants::MILLISECONDS_MASK);

    std::cout << "ET: " << expectedTime << std::endl;

    auto expectedTimeSequence =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              OrderConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              OrderConstants::MILLISECONDS_SHIFT)),
                     o.getSequence());

    std::cout << "ETS: " << expectedTimeSequence << std::endl;

    REQUIRE(time == o.getTime());
    REQUIRE(time == expectedTime);
    REQUIRE(newTimeSequence == expectedTimeSequence);
}

TEST_CASE("Order::setTime() shouldn't change Index") {

    auto o = Order("AAPL").withSequence(123);

    auto oldIndex = o.getIndex();

    std::int64_t time = 1'701'703'226'500LL;

    o.setTime(time);

    REQUIRE(oldIndex == o.getIndex());
}

TEST_CASE("Order::setTime() shouldn't change Sequence") {

    auto o = Order("AAPL").withSequence(123);

    auto oldSequence = o.getSequence();

    std::int64_t time = 1'701'703'226'500LL;

    o.setTime(time);

    REQUIRE(oldSequence == o.getSequence());
}

TEST_CASE("Order::setIndex() shouldn't change Sequence") {

    auto o = Order("AAPL").withSequence(123);

    auto oldSequence = o.getSequence();

    o.setIndex(1'234'567'891'011LL);

    REQUIRE(oldSequence == o.getSequence());
}