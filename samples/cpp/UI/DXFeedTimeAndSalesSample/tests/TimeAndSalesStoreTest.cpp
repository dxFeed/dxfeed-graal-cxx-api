// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "TimeAndSalesStore.hpp"

#include <cstdint>
#include <vector>

using dxfeed::time_and_sales_sample::TimeAndSaleRow;
using dxfeed::time_and_sales_sample::TimeAndSalesStore;

namespace {

TimeAndSaleRow row(std::int64_t index, double price = 0.0, bool remove = false) {
    return TimeAndSaleRow{.time = index / 10, .index = index, .price = price, .remove = remove};
}

} // namespace

TEST_CASE("a snapshot replaces the previous contents") {
    TimeAndSalesStore store{30};
    store.apply({row(10), row(20)}, false);
    store.apply({row(30), row(40)}, true);

    const auto snapshot = store.snapshot();
    REQUIRE(snapshot.size() == 2);
    CHECK(snapshot[0].index == 30);
    CHECK(snapshot[1].index == 40);
}

TEST_CASE("updates replace and remove rows by index") {
    TimeAndSalesStore store{30};
    store.apply({row(10, 1.0), row(20, 2.0)}, true);
    store.apply({row(10, 3.0), row(20, 0.0, true)}, false);

    const auto snapshot = store.snapshot();
    REQUIRE(snapshot.size() == 1);
    CHECK(snapshot[0].index == 10);
    CHECK(snapshot[0].price == doctest::Approx(3.0));
}

TEST_CASE("only the newest configured number of trades is retained") {
    TimeAndSalesStore store{3};
    store.apply({row(50), row(10), row(40), row(30), row(20)}, true);

    const auto snapshot = store.snapshot();
    REQUIRE(snapshot.size() == 3);
    CHECK(snapshot[0].index == 30);
    CHECK(snapshot[1].index == 40);
    CHECK(snapshot[2].index == 50);
}

TEST_CASE("presentation order uses time and index from oldest to newest") {
    TimeAndSalesStore store{30};
    auto first = row(100);
    first.time = 20;
    auto second = row(200);
    second.time = 10;
    store.apply({first, second}, true);

    const auto snapshot = store.snapshot();
    REQUIRE(snapshot.size() == 2);
    CHECK(snapshot[0].index == 200);
    CHECK(snapshot[1].index == 100);
}
