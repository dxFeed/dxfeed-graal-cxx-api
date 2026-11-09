// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// MDAPI-429: the functions of the public headers that are defined in the library must be exported from it. With
// DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS=ON (the dynamic-link-tests CI job) a missing export is a link error of this test.

#include <dxfeed_graal_cpp_api/api.hpp>
#include "../../src/internal/Isolate.hpp"
#include "../../src/internal/utils/StringUtils.hpp"

#include <doctest.h>

#include <string>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;
using namespace dxfcpp::literals;

TEST_CASE("StringHash and iEquals are exported") {
    CHECK(StringHash{}("abc") == StringHash{}(std::string("abc")));
    CHECK(StringHash{}("abc") == StringHash{}(std::string_view("abc")));
    CHECK(iEquals("ABC", "abc"));
}

TEST_CASE("EventFlagsMask::contains is exported") {
    const EventFlagsMask mask{EventFlag::SNAPSHOT_BEGIN, EventFlag::SNAPSHOT_END};

    CHECK(mask.contains(EventFlag::SNAPSHOT_BEGIN));
    CHECK_FALSE(mask.contains(EventFlag::REMOVE_EVENT));
}

TEST_CASE("The _c literal is exported") {
    CHECK("AAPL{=d}"_c == CandleSymbol::valueOf("AAPL{=d}"));
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "DXFeedTimeSeriesSubscription is exported") {
    const auto subscription = feed->createTimeSeriesSubscription(TimeAndSale::TYPE);

    subscription->setFromTime(42);

    CHECK(subscription->getFromTime() == 42);
}

TEST_CASE("System::setIsolateArguments is exported") {
    // Empty arguments change nothing; the result tells whether the isolate was created before the call.
    const bool created = Isolate::isCreated();

    CHECK(System::setIsolateArguments({}) == !created);
}
