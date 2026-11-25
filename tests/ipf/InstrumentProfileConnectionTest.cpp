// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TapeSupport.hpp"
#include "../support/TestSupport.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

namespace {

std::size_t viewSize(const InstrumentProfileCollector &collector) {
    std::size_t result = 0;
    const auto view = collector.view();

    while (view->hasNext()) {
        static_cast<void>(view->next());
        result++;
    }

    return result;
}

} // namespace

TEST_CASE("InstrumentProfileConnection reads a file into the collector and reports the states") {
    using State = InstrumentProfileConnection::State;

    const auto collector = InstrumentProfileCollector::create();
    const auto address = test::testDataPath("ipf/option-chains.ipf");
    const auto connection = InstrumentProfileConnection::createConnection(address, collector);
    std::mutex mutex{};
    std::vector<State> newStates{};

    const auto listener = connection->addStateChangeListener([&](State, State newState) {
        std::lock_guard lock{mutex};

        newStates.push_back(newState);
    });

    CHECK(connection->getAddress() == address);
    CHECK(connection->getState() == State::NOT_CONNECTED);

    connection->start();

    REQUIRE(connection->waitUntilCompleted(10s));
    CHECK(connection->getState() == State::COMPLETED);

    // The connection can report COMPLETED before it stores the modification time of the file.
    CHECK(test::waitUntil([&] {
        return connection->getLastModified() > 0;
    }));
    CHECK(viewSize(*collector) == 14);

    CHECK(test::waitUntil([&] {
        std::lock_guard lock{mutex};

        return std::find(newStates.begin(), newStates.end(), State::COMPLETED) != newStates.end();
    }));

    connection->close();
    CHECK(connection->getState() == State::CLOSED);

    connection->removeStateChangeListener(listener);
}

TEST_CASE("InstrumentProfileConnection keeps the update period") {
    const auto collector = InstrumentProfileCollector::create();
    const auto connection =
        InstrumentProfileConnection::createConnection(test::testDataPath("ipf/option-chains.ipf"), collector);

    connection->setUpdatePeriod(12345);
    CHECK(connection->getUpdatePeriod() == 12345);

    connection->setUpdatePeriod(2s);
    CHECK(connection->getUpdatePeriodAsDuration() == 2s);
}

TEST_CASE("A closed InstrumentProfileConnection with a state listener can be destroyed") {
    const auto collector = InstrumentProfileCollector::create();
    auto connection =
        InstrumentProfileConnection::createConnection(test::testDataPath("ipf/option-chains.ipf"), collector);

    static_cast<void>(connection->addStateChangeListener([](auto, auto) {
    }));

    connection->start();
    connection->close();
    connection.reset();

    // The destructor removes the listener and closes the connection again; the collector stays usable.
    CHECK_NOTHROW(static_cast<void>(viewSize(*collector)));
}
