// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

TEST_CASE("OrderSource::valueOf the method should work correctly with predefined sources") {
    REQUIRE(OrderSource::valueOf("COMPOSITE_ASK") == OrderSource::COMPOSITE_ASK);
    REQUIRE(OrderSource::valueOf("NTV") == OrderSource::NTV);
    REQUIRE(OrderSource::valueOf("NTV") != OrderSource::ntv);
    REQUIRE(OrderSource::valueOf("NTV2") != OrderSource::NTV);
}

// The expected values below are the results of the Java API (QD 3.355) for the same calls.

TEST_CASE("OrderSource::valueOf(id) returns the predefined sources and the sources of alphanumeric names") {
    const std::vector<std::pair<std::int32_t, std::string>> expected{
        {0, "DEFAULT"},      {1, "COMPOSITE_BID"}, {2, "COMPOSITE_ASK"}, {3, "REGIONAL_BID"},
        {4, "REGIONAL_ASK"}, {5, "AGGREGATE_BID"}, {6, "AGGREGATE_ASK"}, {7, "COMPOSITE"},
        {8, "REGIONAL"},     {0x4E5456, "NTV"},    {0x41424344, "ABCD"}};

    for (const auto &[id, name] : expected) {
        CAPTURE(id);

        const auto &source = OrderSource::valueOf(id);

        CHECK(source.id() == id);
        CHECK(source.name() == name);
    }
}

TEST_CASE("OrderSource::valueOf(id) rejects the ids that are not names of 1-4 alphanumeric characters") {
    for (const std::int32_t id : {-1, 42, 0x3F, 0x40, 0x7FFF'FFFF, static_cast<std::int32_t>(0x8000'0000), 0x0100'0000,
                                  0x0020'2020, 0x007F'7F7F, 0x4100'0000}) {
        CAPTURE(id);
        CHECK_THROWS(OrderSource::valueOf(id));
    }
}

TEST_CASE("OrderSource::valueOf(name) returns the predefined sources and the sources of 1-4 alphanumeric characters") {
    const std::vector<std::pair<std::string, std::int32_t>> expected{
        {"A", 65}, {"ABCD", 0x41424344}, {"ntv", 7'238'774}, {"NTV", 0x4E5456}, {"12", 12'594},
        {"1", 49}, {"COMPOSITE_BID", 1}, {"DEFAULT", 0}};

    for (const auto &[name, id] : expected) {
        CAPTURE(name);

        const auto source = OrderSource::valueOf(name);

        CHECK(source.id() == id);
        CHECK(source.name() == name);
    }
}

TEST_CASE("OrderSource::valueOf(name) rejects other names") {
    // "\xC3\x84\x42" is U+00C4 U+0042 in UTF-8.
    for (const std::string name : {"", "ABCDE", "a b", "\xC3\x84\x42", "EMPTY", "A-B", "~"}) {
        CAPTURE(name);
        CHECK_THROWS(OrderSource::valueOf(name));
    }
}

TEST_CASE("OrderSource::isSpecialSourceId is true for the ids 1-9") {
    for (std::int32_t id = -1; id <= 11; id++) {
        CAPTURE(id);
        CHECK(OrderSource::isSpecialSourceId(id) == (id >= 1 && id <= 9));
    }
}

TEST_CASE("OrderBase::setSource should preserve the low index bits") {
    constexpr std::int64_t index = 123'456'789LL;
    auto order = Order("AAPL");

    order.setIndex(index);
    order.setSource(OrderSource::NTV);

    CHECK(order.getSource() == OrderSource::NTV);
    CHECK((order.getIndex() & 0xffff'ffffLL) == index);
}

// MDAPI-429: these functions throw on invalid input, as in Java (IllegalArgumentException); with noexcept the process
// was terminated.
static_assert(!noexcept(std::declval<const OrderBase &>().getSource()));
static_assert(!noexcept(std::declval<OrderBase &>().setExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<Order &>().withIndex(0)));
static_assert(!noexcept(std::declval<Order &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<Order &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<SpreadOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<SpreadOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<SpreadOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(CmdArgsUtils::parseEventSources("")));

TEST_CASE("OrderBase::getSource throws InvalidArgumentException for an invalid source id in the index") {
    auto order = Order("AAPL");

    order.setIndex(0x00FFLL << 48);

    CHECK_THROWS_AS(order.getSource(), InvalidArgumentException);
}

TEST_CASE("OrderBase::setExchangeCode throws InvalidArgumentException for a code that is not a 7-bit character") {
    auto order = Order("AAPL");

    CHECK_THROWS_AS(order.setExchangeCode(std::int16_t{0xE2}), InvalidArgumentException);
    CHECK_THROWS_AS(order.setExchangeCode(static_cast<char>(0xE2)), InvalidArgumentException);

    order.setExchangeCode(std::int16_t{'Q'});

    CHECK(order.getExchangeCode() == 'Q');
}

template <typename O> void checkOrderWithThrows() {
    auto order = O("AAPL");

    CHECK_THROWS_AS(order.withIndex(-1), InvalidArgumentException);
    CHECK_THROWS_AS(order.withExchangeCode(static_cast<char>(0xE2)), InvalidArgumentException);
    CHECK_THROWS_AS(order.withExchangeCode(std::int16_t{0xE2}), InvalidArgumentException);
    CHECK(order.withIndex(1).withExchangeCode('Q').getExchangeCode() == 'Q');
}

TEST_CASE("withIndex and withExchangeCode of the order events throw InvalidArgumentException for invalid values") {
    SUBCASE("Order") {
        checkOrderWithThrows<Order>();
    }

    SUBCASE("SpreadOrder") {
        checkOrderWithThrows<SpreadOrder>();
    }

    SUBCASE("AnalyticOrder") {
        checkOrderWithThrows<AnalyticOrder>();
    }

    SUBCASE("OtcMarketsOrder") {
        checkOrderWithThrows<OtcMarketsOrder>();
    }
}

TEST_CASE("CmdArgsUtils::parseEventSources throws InvalidArgumentException for an invalid source name") {
    CHECK_THROWS_AS(CmdArgsUtils::parseEventSources("A-B"), InvalidArgumentException);
    CHECK_THROWS_AS(CmdArgsUtils::parseEventSources("ABCDE"), InvalidArgumentException);
    CHECK(CmdArgsUtils::parseEventSources("NTV, ntv").size() == 2);
}
