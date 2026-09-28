// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Events of all types through QD text tapes. The golden tapes in tests/data/tapes are the reference: the text format
// is QD's own, and the Java API reads and writes them the same way (checked with QD 3.355, see the README there).
//  - C++ -> Java -> C++: the events of the factory are written to a tape and read back.
//  - Java -> C++: the golden tape is read into C++ events.
//  - Java -> C++ -> Java: every column of the golden tape survives reading into C++ and writing back.
//  - Defaults: default-constructed C++ events are written as the Java API writes default-constructed events.
// Known defects are listed below; the tests for them are `may_fail` until the defects are fixed.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/EventFactory.hpp"
#include "../support/TapeSupport.hpp"

#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

// Trade, TradeETH and TimeAndSale have no trade id in the C++ API (static analysis report, EVT-2).
const std::vector<std::string> KNOWN_ROUND_TRIP_DEFECTS{"Trade.TradeId", "TradeETH.TradeId", "TimeAndSale.TradeId"};

// Java defaults: the day volume and turnover of Trade and TradeETH are NaN, the text of TextMessage is null
// (static analysis report, EVT-16).
const std::vector<std::string> KNOWN_DEFAULTS_DEFECTS{"Trade.DayVolume", "Trade.DayTurnover", "TradeETH.DayVolume",
                                                      "TradeETH.DayTurnover", "TextMessage.Text"};

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

TEST_CASE("Every column of the golden tape survives reading into C++ and writing back, except known defects") {
    test::checkDifferences(goldenRoundTripDifferences(), KNOWN_ROUND_TRIP_DEFECTS, false);
}

TEST_CASE("Every column of the golden tape survives reading into C++ and writing back (EVT-2)" * doctest::may_fail()) {
    test::checkDifferences(goldenRoundTripDifferences(), KNOWN_ROUND_TRIP_DEFECTS, true);
}

TEST_CASE("Default events are written as the Java API writes them, except known defects") {
    test::checkDifferences(defaultsDifferences(), KNOWN_DEFAULTS_DEFECTS, false);
}

TEST_CASE("Default events are written as the Java API writes them (EVT-16)" * doctest::may_fail()) {
    test::checkDifferences(defaultsDifferences(), KNOWN_DEFAULTS_DEFECTS, true);
}
