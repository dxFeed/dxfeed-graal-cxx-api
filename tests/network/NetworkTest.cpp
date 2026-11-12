// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Tests over a network connection to a QDS server that sends tests/data/tapes/network.txt. They are built only with
// DXFCXX_ENABLE_NETWORK_TESTS=ON (CTest label: network). On Linux CTest starts the local server itself (the fixture
// of local-server.sh); elsewhere, or for another server, DXFCXX_NETWORK_TEST_ADDRESS gives the address of a server
// that sends the same tape. The events are compared with network/NetworkTapeData.hpp.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"
#include "NetworkTapeData.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#ifndef DXFCXX_NETWORK_TEST_PORT
#    error "DXFCXX_NETWORK_TEST_PORT must be the port of the local server"
#endif

using namespace dxfcpp;
using namespace std::literals;
namespace tape = dxfcpp::test::network_tape;

namespace {

constexpr auto WAIT_TIMEOUT = 30s;

std::string serverAddress() {
    if (const auto *address = std::getenv("DXFCXX_NETWORK_TEST_ADDRESS"); address != nullptr && *address != '\0') {
        return address;
    }

    return "127.0.0.1:" + std::to_string(DXFCXX_NETWORK_TEST_PORT);
}

// The index of an order without the source, which QD keeps in the high bits.
std::int64_t orderIndex(const std::shared_ptr<Order> &order) {
    return order->getIndex() & 0xFFFF'FFFFLL;
}

void checkOrders(std::vector<std::shared_ptr<Order>> actual, std::vector<std::shared_ptr<Order>> expected) {
    const auto byIndex = [](const auto &a, const auto &b) {
        return orderIndex(a) < orderIndex(b);
    };

    std::sort(actual.begin(), actual.end(), byIndex);
    std::sort(expected.begin(), expected.end(), byIndex);
    REQUIRE(actual.size() == expected.size());

    for (std::size_t i = 0; i < expected.size(); i++) {
        CAPTURE(expected[i]->toString());
        CAPTURE(actual[i]->toString());
        CHECK(orderIndex(actual[i]) == orderIndex(expected[i]));
        CHECK(actual[i]->getSource() == OrderSource::NTV);
        CHECK(actual[i]->getTime() == expected[i]->getTime());
        CHECK(actual[i]->getPrice() == expected[i]->getPrice());
        CHECK(actual[i]->getSize() == expected[i]->getSize());
        CHECK(actual[i]->getOrderSide() == expected[i]->getOrderSide());
        CHECK(actual[i]->getExchangeCode() == expected[i]->getExchangeCode());
    }
}

void checkCandles(std::vector<std::shared_ptr<Candle>> actual, std::vector<std::shared_ptr<Candle>> expected) {
    const auto byTime = [](const auto &a, const auto &b) {
        return a->getTime() < b->getTime();
    };

    std::sort(actual.begin(), actual.end(), byTime);
    std::sort(expected.begin(), expected.end(), byTime);
    REQUIRE(actual.size() == expected.size());

    for (std::size_t i = 0; i < expected.size(); i++) {
        CAPTURE(expected[i]->toString());
        CAPTURE(actual[i]->toString());
        CHECK(actual[i]->getTime() == expected[i]->getTime());
        CHECK(actual[i]->getCount() == expected[i]->getCount());
        CHECK(actual[i]->getOpen() == expected[i]->getOpen());
        CHECK(actual[i]->getHigh() == expected[i]->getHigh());
        CHECK(actual[i]->getLow() == expected[i]->getLow());
        CHECK(actual[i]->getClose() == expected[i]->getClose());
        CHECK(actual[i]->getVolume() == expected[i]->getVolume());
        CHECK(actual[i]->getVWAP() == expected[i]->getVWAP());
    }
}

// The candles of the tape not older than fromTime.
std::vector<std::shared_ptr<Candle>> candlesFrom(std::int64_t fromTime) {
    auto candles = tape::createCandles();

    candles.erase(std::remove_if(candles.begin(), candles.end(),
                                 [&](const auto &candle) {
                                     return candle->getTime() < fromTime;
                                 }),
                  candles.end());

    return candles;
}

// Subscribes a TimeSeriesTxModel to the daily candles from fromTime and checks its first snapshot.
void checkCandleSnapshot(std::int64_t fromTime) {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(serverAddress());
    std::mutex mutex{};
    std::vector<std::shared_ptr<Candle>> snapshot{};
    std::atomic<bool> snapshotReceived{false};
    const auto model = TimeSeriesTxModel<Candle>::newBuilder()
                           ->withFeed(endpoint->getFeed())
                           ->withBatchProcessing(true)
                           ->withSnapshotProcessing(true)
                           ->withFromTime(fromTime)
                           ->withListener(TimeSeriesTxModelListener<Candle>::create(
                               [&](const std::vector<std::shared_ptr<Candle>> &events, bool isSnapshot) {
                                   if (isSnapshot && !snapshotReceived) {
                                       std::lock_guard lock{mutex};

                                       snapshot = events;
                                       snapshotReceived = true;
                                   }
                               }))
                           ->withSymbol(CandleSymbol::valueOf(tape::CANDLE_SYMBOL))
                           ->build();

    const auto received = test::waitUntil(
        [&] {
            return snapshotReceived.load();
        },
        WAIT_TIMEOUT);

    model->close();
    endpoint->close();

    REQUIRE(received);

    std::lock_guard lock{mutex};

    checkCandles(snapshot, candlesFrom(fromTime));
}

} // namespace

TEST_CASE("A feed endpoint connects to the server") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    endpoint->connect(serverAddress());
    REQUIRE(test::waitUntil(
        [&] {
            return endpoint->getState() == DXEndpoint::State::CONNECTED;
        },
        WAIT_TIMEOUT));

    endpoint->disconnect();
    REQUIRE(endpoint->getState() == DXEndpoint::State::NOT_CONNECTED);

    endpoint->close();
    REQUIRE(endpoint->getState() == DXEndpoint::State::CLOSED);
}

TEST_CASE("getLastEventsPromises receives the quotes of the tape") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(serverAddress());
    const auto expected = tape::createQuotes();
    const auto promises = endpoint->getFeed()->getLastEventsPromises<Quote>({"AAPL", "IBM"});
    const auto received = Promises::allOf(promises)->awaitWithoutException(WAIT_TIMEOUT);

    endpoint->close();
    REQUIRE(received);
    REQUIRE(promises->size() == expected.size());

    for (std::size_t i = 0; i < expected.size(); i++) {
        const auto actual = (*promises)[i].getResult();

        CAPTURE(expected[i]->toString());
        REQUIRE(actual != nullptr);
        CAPTURE(actual->toString());
        CHECK(actual->getEventSymbol() == expected[i]->getEventSymbol());
        CHECK(actual->getBidTime() == expected[i]->getBidTime());
        CHECK(actual->getBidExchangeCode() == expected[i]->getBidExchangeCode());
        CHECK(actual->getBidPrice() == expected[i]->getBidPrice());
        CHECK(actual->getBidSize() == expected[i]->getBidSize());
        CHECK(actual->getAskTime() == expected[i]->getAskTime());
        CHECK(actual->getAskExchangeCode() == expected[i]->getAskExchangeCode());
        CHECK(actual->getAskPrice() == expected[i]->getAskPrice());
        CHECK(actual->getAskSize() == expected[i]->getAskSize());
    }
}

TEST_CASE("IndexedTxModel receives the order snapshot of the tape in one transaction") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(serverAddress());
    std::mutex mutex{};
    std::vector<std::shared_ptr<Order>> snapshot{};
    std::atomic<int> snapshotCalls{0};
    const auto model = IndexedTxModel<Order>::newBuilder()
                           ->withFeed(endpoint->getFeed())
                           ->withBatchProcessing(true)
                           ->withSnapshotProcessing(true)
                           ->withSources({OrderSource::NTV})
                           ->withListener([&](const auto &, const std::vector<std::shared_ptr<Order>> &events,
                                              bool isSnapshot) {
                               if (isSnapshot) {
                                   std::lock_guard lock{mutex};

                                   snapshot = events;
                                   ++snapshotCalls;
                               }
                           })
                           ->withSymbol(tape::ORDER_SYMBOL)
                           ->build();

    const auto received = test::waitUntil(
        [&] {
            return snapshotCalls.load() > 0;
        },
        WAIT_TIMEOUT);

    model->close();
    endpoint->close();

    REQUIRE(received);
    CHECK(snapshotCalls.load() == 1);

    std::lock_guard lock{mutex};

    checkOrders(snapshot, tape::createOrders());
}

TEST_CASE("TimeSeriesTxModel receives the candles of the tape from a time inside the tape") {
    // The 20 newest candles.
    checkCandleSnapshot(tape::candleTime(tape::CANDLE_COUNT - 20));
}

TEST_CASE("TimeSeriesTxModel receives all the candles of the tape from a time older than the tape") {
    // The snapshot ends on the SNAPSHOT_SNIP of the oldest candle: there is nothing older to wait for.
    checkCandleSnapshot(tape::FIRST_CANDLE_TIME - tape::DAY);
}
