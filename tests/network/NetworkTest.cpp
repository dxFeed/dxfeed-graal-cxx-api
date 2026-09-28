// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Smoke tests against the public demo feed. They need external network access and are built only with
// DXFCXX_ENABLE_NETWORK_TESTS=ON (CTest label: network). The hermetic variants of these scenarios are in the other
// tests.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <chrono>
#include <limits>

using namespace dxfcpp;
using namespace std::literals;

namespace {

constexpr auto DEMO_ADDRESS = "demo.dxfeed.com:7300";

} // namespace

TEST_CASE("A feed endpoint connects to the demo feed") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    endpoint->connect(DEMO_ADDRESS);
    REQUIRE(test::waitUntil(
        [&] {
            return endpoint->getState() == DXEndpoint::State::CONNECTED;
        },
        30s));

    endpoint->disconnect();
    REQUIRE(endpoint->getState() == DXEndpoint::State::NOT_CONNECTED);

    endpoint->close();
    REQUIRE(endpoint->getState() == DXEndpoint::State::CLOSED);
}

TEST_CASE("getLastEventsPromises receives quotes from the demo feed") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(DEMO_ADDRESS);
    const auto promises = endpoint->getFeed()->getLastEventsPromises<Quote>({"AAPL", "IBM"});

    REQUIRE(Promises::allOf(promises)->awaitWithoutException(30s));

    for (const auto &promise : *promises) {
        REQUIRE(promise.getResult() != nullptr);
    }

    endpoint->close();
}

TEST_CASE("IndexedTxModel receives a snapshot of AAPL orders from the demo feed") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(DEMO_ADDRESS);
    std::atomic<bool> snapshotReceived{false};
    const auto model = IndexedTxModel<Order>::newBuilder()
                           ->withFeed(endpoint->getFeed())
                           ->withBatchProcessing(true)
                           ->withSnapshotProcessing(true)
                           ->withSources({OrderSource::NTV})
                           ->withListener([&](const auto &, const auto &, bool isSnapshot) {
                               if (isSnapshot) {
                                   snapshotReceived = true;
                               }
                           })
                           ->withSymbol("AAPL")
                           ->build();

    REQUIRE(test::waitUntil(
        [&] {
            return snapshotReceived.load();
        },
        30s));

    model->close();
    endpoint->close();
}

TEST_CASE("TimeSeriesTxModel receives a snapshot of AAPL candles from the demo feed") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(DEMO_ADDRESS);
    std::atomic<bool> snapshotReceived{false};
    const auto model = TimeSeriesTxModel<Candle>::newBuilder()
                           ->withFeed(endpoint->getFeed())
                           ->withBatchProcessing(true)
                           ->withSnapshotProcessing(true)
                           ->withFromTime(std::chrono::milliseconds(now()) - std::chrono::days(3))
                           ->withListener(TimeSeriesTxModelListener<Candle>::create([&](const auto &, bool isSnapshot) {
                               if (isSnapshot) {
                                   snapshotReceived = true;
                               }
                           }))
                           ->withSymbol(CandleSymbol::valueOf("AAPL&Q{=1d}"))
                           ->build();

    REQUIRE(test::waitUntil(
        [&] {
            return snapshotReceived.load();
        },
        30s));

    model->close();
    endpoint->close();
}
