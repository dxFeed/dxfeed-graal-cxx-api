// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The tape of the network tests (tests/data/tapes/network.txt) is generated from network/NetworkTapeData.hpp. These
// tests keep the two in sync: a change of the events without the regenerated tape, or of the tape by hand, fails here
// and not only in the network tests, which run on Linux only.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../network/NetworkTapeData.hpp"
#include "../support/TapeSupport.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

constexpr auto REGENERATE_HINT = "the events of network/NetworkTapeData.hpp and tests/data/tapes/network.txt differ: "
                                 "regenerate the tape (tests/data/tapes/README.md)";

template <typename Event>
std::vector<std::string> toStrings(const std::vector<std::shared_ptr<EventType>> &events) {
    std::vector<std::string> result{};

    for (const auto &event : events) {
        if (const auto typed = event->sharedAs<Event>()) {
            result.push_back(typed->toString());
        }
    }

    return result;
}

template <typename Event>
std::vector<std::string> toStrings(const std::vector<std::shared_ptr<Event>> &events) {
    std::vector<std::string> result{};

    for (const auto &event : events) {
        result.push_back(event->toString());
    }

    return result;
}

} // namespace

TEST_CASE("The network tape is the tape that the generator writes") {
    const auto generated = test::temporaryFilePath("network.txt");

    test::network_tape::writeTape(generated);

    const auto expected = test::parseTextTape(test::testDataPath(test::network_tape::TAPE_FILE));
    const auto actual = test::parseTextTape(generated);

    INFO(REGENERATE_HINT);
    REQUIRE(actual.size() == expected.size());

    // The records are compared in the order of the file; the times are compared as instants (TapeSupport.hpp).
    for (std::size_t i = 0; i < expected.size(); i++) {
        CAPTURE(i);
        CAPTURE(expected[i].name);
        REQUIRE(actual[i].name == expected[i].name);
        REQUIRE(actual[i].fields == expected[i].fields);
    }
}

TEST_CASE("The network tape is read as the events of NetworkTapeData.hpp") {
    const auto events = test::readTape(test::testDataPath(test::network_tape::TAPE_FILE),
                                       {test::network_tape::CANDLE_SYMBOL});

    INFO(REGENERATE_HINT);
    CHECK(toStrings<Quote>(events) == toStrings(test::network_tape::createQuotes()));
    CHECK(toStrings<Order>(events) == toStrings(test::network_tape::createOrders()));
    CHECK(toStrings<Candle>(events) == toStrings(test::network_tape::createCandles()));
}
