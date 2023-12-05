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

struct OptionSaleConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
};

TEST_CASE("OptionSale::setTime() should change TimeSequence") {
    auto os = OptionSale("AAPL");

    os.setSequence(123);

    std::int64_t time = 1'701'703'226'500LL;

    os.setTime(time);

    auto newTimeSequence = os.getTimeSequence();
    auto expectedTimeSequence =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              OptionSaleConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              OptionSaleConstants::MILLISECONDS_SHIFT)),
                     os.getSequence());
    auto expectedTime = dxfcpp::sar(expectedTimeSequence, OptionSaleConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(expectedTimeSequence, OptionSaleConstants::MILLISECONDS_SHIFT),
                                      OptionSaleConstants::MILLISECONDS_MASK);

    REQUIRE(time == os.getTime());
    REQUIRE(time == expectedTime);
    REQUIRE(newTimeSequence == expectedTimeSequence);
}

TEST_CASE("OptionSale::setTime() shouldn't change Index") {
    auto os = OptionSale("AAPL");

    os.setSequence(123);

    auto oldIndex = os.getIndex();
    std::int64_t time = 1'701'703'226'500LL;

    os.setTime(time);

    REQUIRE(oldIndex == os.getIndex());
}

TEST_CASE("OptionSale::setTime() shouldn't change Sequence") {
    auto os = OptionSale("AAPL");

    os.setSequence(123);
    auto oldSequence = os.getSequence();
    std::int64_t time = 1'701'703'226'500LL;

    os.setTime(time);

    REQUIRE(oldSequence == os.getSequence());
}

TEST_CASE("OptionSale::setIndex() shouldn't change Sequence") {
    auto os = OptionSale("AAPL");

    os.setSequence(123);
    auto oldSequence = os.getSequence();

    os.setIndex(1'234'567'891'011LL);

    REQUIRE(oldSequence == os.getSequence());
}

struct OrderConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
};

TEST_CASE("Order::setTime() should change TimeSequence") {
    auto o = Order("AAPL").withSequence(123);
    std::int64_t time = 1'701'703'226'500LL;

    o.setTime(time);

    auto newTimeSequence = o.getTimeSequence();
    auto expectedTimeSequence =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              OrderConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              OrderConstants::MILLISECONDS_SHIFT)),
                     o.getSequence());
    auto expectedTime = dxfcpp::sar(expectedTimeSequence, OrderConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(expectedTimeSequence, OrderConstants::MILLISECONDS_SHIFT),
                                      OrderConstants::MILLISECONDS_MASK);

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

struct TimeAndSaleConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
};

TEST_CASE("TimeAndSale::setTime() should change Index") {
    auto tns = TimeAndSale("AAPL");
    std::int64_t time = 1'701'703'226'500LL;

    tns.setTime(time);

    auto newIndex = tns.getIndex();
    auto expectedIndex =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              TimeAndSaleConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              TimeAndSaleConstants::MILLISECONDS_SHIFT)),
                     tns.getSequence());
    auto expectedTime = dxfcpp::sar(expectedIndex, TimeAndSaleConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(expectedIndex, TimeAndSaleConstants::MILLISECONDS_SHIFT),
                                      TimeAndSaleConstants::MILLISECONDS_MASK);

    REQUIRE(time == tns.getTime());
    REQUIRE(time == expectedTime);
    REQUIRE(newIndex == expectedIndex);
}

TEST_CASE("TimeAndSale::setSequence() should change Index") {
    auto tns = TimeAndSale("AAPL");
    auto oldIndex = tns.getIndex();
    int sequence = 567;

    tns.setSequence(sequence);

    auto expectedIndex = dxfcpp::orOp(dxfcpp::andOp(oldIndex, ~TimeAndSaleConstants::MAX_SEQUENCE), sequence);

    REQUIRE(tns.getIndex() == expectedIndex);
}

struct GreeksConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
};

TEST_CASE("Greeks::setTime() should change Index") {
    auto g = Greeks("AAPL");
    std::int64_t time = 1'701'703'226'500LL;

    g.setTime(time);

    auto newIndex = g.getIndex();
    auto expectedIndex =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              GreeksConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              GreeksConstants::MILLISECONDS_SHIFT)),
                     g.getSequence());
    auto expectedTime = dxfcpp::sar(expectedIndex, GreeksConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(expectedIndex, GreeksConstants::MILLISECONDS_SHIFT),
                                      GreeksConstants::MILLISECONDS_MASK);

    REQUIRE(time == g.getTime());
    REQUIRE(time == expectedTime);
    REQUIRE(newIndex == expectedIndex);
}

TEST_CASE("Greeks::setSequence() should change Index") {
    auto g = Greeks("AAPL");
    auto oldIndex = g.getIndex();
    int sequence = 567;

    g.setSequence(sequence);

    auto expectedIndex = dxfcpp::orOp(dxfcpp::andOp(oldIndex, ~GreeksConstants::MAX_SEQUENCE), sequence);

    REQUIRE(g.getIndex() == expectedIndex);
}

struct SeriesConstants {
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;
};


TEST_CASE("Series::setTime() should change TimeSequence") {
    auto s = Series("AAPL").withSequence(123);
    std::int64_t time = 1'701'703'226'500LL;

    s.setTime(time);

    auto newTimeSequence = s.getTimeSequence();
    auto expectedTimeSequence =
        dxfcpp::orOp(dxfcpp::orOp(dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getSecondsFromTime(time)),
                                              SeriesConstants::SECONDS_SHIFT),
                                  dxfcpp::sal(static_cast<std::int64_t>(dxfcpp::time_util::getMillisFromTime(time)),
                                              SeriesConstants::MILLISECONDS_SHIFT)),
                     s.getSequence());
    auto expectedTime = dxfcpp::sar(expectedTimeSequence, SeriesConstants::SECONDS_SHIFT) * 1000 +
                        dxfcpp::andOp(dxfcpp::sar(expectedTimeSequence, SeriesConstants::MILLISECONDS_SHIFT),
                                      SeriesConstants::MILLISECONDS_MASK);

    REQUIRE(newTimeSequence == expectedTimeSequence);
    REQUIRE(time == expectedTime);
    REQUIRE(time == s.getTime());
}

TEST_CASE("Series::setTime() shouldn't change Index") {
    auto s = Series("AAPL").withSequence(123);
    auto oldIndex = s.getIndex();
    std::int64_t time = 1'701'703'226'500LL;

    s.setTime(time);

    REQUIRE(oldIndex == s.getIndex());
}

TEST_CASE("Series::setTime() shouldn't change Sequence") {
    auto s = Series("AAPL").withSequence(123);
    auto oldSequence = s.getSequence();
    std::int64_t time = 1'701'703'226'500LL;

    s.setTime(time);

    REQUIRE(oldSequence == s.getSequence());
}

TEST_CASE("Series::setIndex() shouldn't change Sequence") {
    auto s = Series("AAPL").withSequence(123);
    auto oldSequence = s.getSequence();

    s.setIndex(1'234'567'891'011LL);

    REQUIRE(oldSequence == s.getSequence());
}