// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Events of all types through QD text tapes. The golden tapes in tests/data/tapes are the reference: the text format
// is QD's own, and the Java API reads and writes them the same way (checked with QD 3.355, and 3.356 for
// OrderImbalance and the Nuam events, see the README there).
//  - C++ -> Java -> C++: the events of the factory are written to a tape and read back.
//  - Java -> C++: the golden tape is read into C++ events.
//  - Java -> C++ -> Java: every column of the golden tape survives reading into C++ and writing back.
//  - Defaults: default-constructed C++ events are written as the Java API writes default-constructed events.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/EventFactory.hpp"
#include "../support/TapeSupport.hpp"

#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

std::vector<std::string> goldenRoundTripDifferences() {
    const auto golden = test::testDataPath("tapes/all-events.txt");
    const auto written = test::temporaryFilePath("golden-round-trip.txt");

    test::writeTextTape(written, test::readTape(golden, {test::tapeCandleSymbol()}));

    return test::compareTapes(golden, written);
}

std::vector<std::string> defaultsDifferences() {
    const auto written = test::temporaryFilePath("defaults.txt");

    test::writeTextTape(written, test::createDefaultEvents());

    return test::compareTapes(test::testDataPath("tapes/defaults.txt"), written);
}

} // namespace

TEST_CASE("Events of all types are unchanged after a round trip through a text tape") {
    const auto expected = test::createTapeEvents();
    const auto path = test::temporaryFilePath("round-trip.txt");

    test::writeTextTape(path, expected);
    test::checkEvents(expected, test::readTape(path, {test::tapeCandleSymbol()}));
}

TEST_CASE("The golden tape is read as the events of the factory") {
    test::checkEvents(test::createTapeEvents(),
                      test::readTape(test::testDataPath("tapes/all-events.txt"), {test::tapeCandleSymbol()}));
}

TEST_CASE("Every column of the golden tape survives reading into C++ and writing back") {
    test::checkDifferences(goldenRoundTripDifferences(), {}, false);
}

TEST_CASE("Default events are written as the Java API writes them") {
    test::checkDifferences(defaultsDifferences(), {}, false);
}

TEST_CASE("A null TextMessage text stays null after a round trip through a text tape, an empty one stays empty") {
    const auto path = test::temporaryFilePath("text-message.txt");

    test::writeTextTape(path,
                        {std::make_shared<TextMessage>("NULL-TEXT"), std::make_shared<TextMessage>("EMPTY-TEXT", "")});

    const auto events = test::readTape(path);

    REQUIRE(events.size() == 2);

    const auto nullText = events[0]->sharedAs<TextMessage>();
    const auto emptyText = events[1]->sharedAs<TextMessage>();

    REQUIRE(nullText);
    REQUIRE(emptyText);
    CHECK(nullText->getEventSymbol() == "NULL-TEXT");
    CHECK(!nullText->getTextOpt());
    CHECK(emptyText->getEventSymbol() == "EMPTY-TEXT");
    CHECK(emptyText->getTextOpt() == std::optional<std::string>{""});
}
