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

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

using namespace dxfcpp;

namespace {

// Trade, TradeETH and TimeAndSale have no trade id in the C++ API (static analysis report, EVT-2).
const std::vector<std::string> KNOWN_ROUND_TRIP_DEFECTS{"Trade.TradeId", "TradeETH.TradeId", "TimeAndSale.TradeId"};

// Java defaults: the day volume and turnover of Trade and TradeETH are NaN, the text of TextMessage is null
// (static analysis report, EVT-16).
const std::vector<std::string> KNOWN_DEFAULTS_DEFECTS{"Trade.DayVolume", "Trade.DayTurnover", "TradeETH.DayVolume",
                                                      "TradeETH.DayTurnover", "TextMessage.Text"};

// The key of an event in a tape: its type and, for the order-like events, its source.
std::string eventKey(const std::shared_ptr<EventType> &event) {
    std::string key = typeid(*event).name();

    if (const auto order = event->sharedAs<OrderBase>()) {
        key += "#" + order->getSource().name();
    }

    return key;
}

// Compares the events by toString(), which prints all the fields of an event.
void checkEvents(const std::vector<std::shared_ptr<EventType>> &expected,
                 const std::vector<std::shared_ptr<EventType>> &actualEvents) {
    std::map<std::string, std::shared_ptr<EventType>> actual{};

    for (const auto &event : actualEvents) {
        actual[eventKey(event)] = event;
    }

    for (const auto &event : expected) {
        const auto key = eventKey(event);

        CAPTURE(key);

        const auto found = actual.find(key);

        REQUIRE(found != actual.end());
        CHECK(found->second->toString() == event->toString());
    }
}

// Returns "Record.Column: expected=..., actual=..." for every difference of two text tapes (EventTime is ignored).
std::vector<std::string> compareTapes(const std::string &expectedPath, const std::string &actualPath) {
    std::map<std::string, test::TapeRecord> actual{};

    for (auto &record : test::parseTextTape(actualPath)) {
        actual[record.name] = std::move(record);
    }

    std::vector<std::string> differences{};

    for (const auto &expected : test::parseTextTape(expectedPath)) {
        const auto found = actual.find(expected.name);

        if (found == actual.end()) {
            differences.push_back(expected.name + ": missing");

            continue;
        }

        for (const auto &[column, value] : expected.fields) {
            if (column == "EventTime") {
                continue;
            }

            const auto actualValue = found->second.fields.find(column);
            const auto actualText = actualValue == found->second.fields.end() ? "<missing>" : actualValue->second;

            if (actualText != value) {
                differences.push_back(expected.name + "." + column + ": expected=" + value + ", actual=" + actualText);
            }
        }

        actual.erase(found);
    }

    for (const auto &[name, record] : actual) {
        differences.push_back(name + ": unexpected");
    }

    return differences;
}

bool isKnown(const std::string &difference, const std::vector<std::string> &known) {
    return std::any_of(known.begin(), known.end(), [&](const auto &prefix) {
        return difference.rfind(prefix + ":", 0) == 0;
    });
}

std::string join(const std::vector<std::string> &lines) {
    std::string result{};

    for (const auto &line : lines) {
        result += "\n  " + line;
    }

    return result;
}

void checkDifferences(const std::vector<std::string> &differences, const std::vector<std::string> &known,
                      bool withKnown) {
    std::vector<std::string> selected{};

    std::copy_if(differences.begin(), differences.end(), std::back_inserter(selected), [&](const auto &difference) {
        return withKnown || !isKnown(difference, known);
    });

    INFO("differences:", join(selected));
    CHECK(selected.empty());
}

std::vector<std::string> goldenRoundTripDifferences() {
    const auto golden = test::testDataPath("tapes/all-events.txt");
    const auto written = test::temporaryFilePath("golden-round-trip.txt");

    test::writeTextTape(written, test::readTape(golden, {test::tapeCandleSymbol()}));

    return compareTapes(golden, written);
}

std::vector<std::string> defaultsDifferences() {
    const auto written = test::temporaryFilePath("defaults.txt");

    test::writeTextTape(written, test::createDefaultEvents());

    return compareTapes(test::testDataPath("tapes/defaults.txt"), written);
}

} // namespace

TEST_CASE("Events of all types are unchanged after a round trip through a text tape") {
    const auto expected = test::createTapeEvents();
    const auto path = test::temporaryFilePath("round-trip.txt");

    test::writeTextTape(path, expected);
    checkEvents(expected, test::readTape(path, {test::tapeCandleSymbol()}));
}

TEST_CASE("The golden tape is read as the events of the factory") {
    checkEvents(test::createTapeEvents(),
                test::readTape(test::testDataPath("tapes/all-events.txt"), {test::tapeCandleSymbol()}));
}

TEST_CASE("Every column of the golden tape survives reading into C++ and writing back, except known defects") {
    checkDifferences(goldenRoundTripDifferences(), KNOWN_ROUND_TRIP_DEFECTS, false);
}

TEST_CASE("Every column of the golden tape survives reading into C++ and writing back (EVT-2)" * doctest::may_fail()) {
    checkDifferences(goldenRoundTripDifferences(), KNOWN_ROUND_TRIP_DEFECTS, true);
}

TEST_CASE("Default events are written as the Java API writes them, except known defects") {
    checkDifferences(defaultsDifferences(), KNOWN_DEFAULTS_DEFECTS, false);
}

TEST_CASE("Default events are written as the Java API writes them (EVT-16)" * doctest::may_fail()) {
    checkDifferences(defaultsDifferences(), KNOWN_DEFAULTS_DEFECTS, true);
}
