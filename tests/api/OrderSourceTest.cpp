// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
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

        const auto &source = OrderSource::valueOf(name);

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
