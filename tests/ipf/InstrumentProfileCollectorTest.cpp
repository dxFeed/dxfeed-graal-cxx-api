// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include <atomic>
#include <memory>
#include <vector>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;

// MDAPI-429: the collector gives the ids of its update listeners itself; each listener has a handler of its own, and
// the ids of the handlers are unique only within a handler.
TEST_CASE("InstrumentProfileCollector gives different ids to the update listeners and calls each of them") {
    const auto collector = InstrumentProfileCollector::create();
    std::atomic<int> firstCalls{};
    std::atomic<int> secondCalls{};

    const auto first = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        ++firstCalls;
    });
    const auto second = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        ++secondCalls;
    });

    CHECK(first != second);

    const auto profile = InstrumentProfile::create();

    profile->setType("STOCK");
    profile->setSymbol("AAPL");
    collector->updateInstrumentProfile(profile);

    CHECK(test::waitUntil([&] {
        return firstCalls > 0 && secondCalls > 0;
    }));

    collector->removeUpdateListener(first);
    collector->removeUpdateListener(second);
}
