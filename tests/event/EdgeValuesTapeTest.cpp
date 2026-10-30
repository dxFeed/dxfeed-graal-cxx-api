// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Edge values of all event types through QD text tapes (test::createEdgeEvents()). The golden tapes in tests/data/tapes
// are written by the Java API: edge-values.txt from the same values, so it shows how Java passes every edge value to QD
// (QD rounds, truncates or drops some of them, and C++ must do the same), and edge-values-round-trip.txt by reading
// edge-values.txt and writing the events back (QD does not read back every value it writes, e.g. the day ids of the
// int32 limits become 0).
//  - C++ -> QD: the edge events written by C++ make the same tape as Java.
//  - Java -> C++ -> Java: the golden tape read into C++ and written back is the tape of the same round trip in Java.
// The differences that the Java API makes itself are listed below.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/EventFactory.hpp"
#include "../support/TapeSupport.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

// Only on reading: the Java API itself reads the int64 minimum of TradeStatTime as that minimum plus the current time in
// milliseconds (the golden round trip has the value of the day it was written), so it is never equal.
const std::vector<std::string> JAVA_ROUND_TRIP_DIFFERENCES{"NuamTrade.TradeStatTime@EDGE-NEG-INF",
                                                           "NuamTrade.TradeStatTime@EDGE-LOWEST"};

std::vector<std::string> writeDifferences() {
    const auto written = test::temporaryFilePath("edge-values.txt");

    test::writeTextTape(written, test::createEdgeEvents());

    return test::compareTapes(test::testDataPath("tapes/edge-values.txt"), written);
}

std::vector<std::string> roundTripDifferences() {
    const auto written = test::temporaryFilePath("edge-values-round-trip.txt");

    test::writeTextTape(written,
                        test::readTape(test::testDataPath("tapes/edge-values.txt"), test::edgeCandleSymbols()));

    return test::compareTapes(test::testDataPath("tapes/edge-values-round-trip.txt"), written);
}

} // namespace

TEST_CASE("Edge values are written as the Java API writes them") {
    test::checkDifferences(writeDifferences(), {}, false);
}

TEST_CASE("The edge-value golden tape read into C++ and written back is the Java round trip") {
    test::checkDifferences(roundTripDifferences(), JAVA_ROUND_TRIP_DIFFERENCES, false);
}

// A char above ASCII ('\xC4'). The Java API writes (char) 0xC4 as `\u00C4`, packs it into the flags of TimeAndSale (the
// trade through exempt) and rejects it in Order::setExchangeCode (QD 3.355).
namespace {

std::map<std::string, test::TapeRecord> writeCharAboveAscii() {
    constexpr char c = '\xC4';
    auto quote = std::make_shared<Quote>("CHAR");
    auto trade = std::make_shared<Trade>("CHAR");
    auto timeAndSale = std::make_shared<TimeAndSale>("CHAR");

    quote->setBidExchangeCode(c);
    trade->setExchangeCode(c);
    timeAndSale->setExchangeCode(c);
    timeAndSale->setTradeThroughExempt(c);

    const auto path = test::temporaryFilePath("char.txt");

    test::writeTextTape(path, {quote, trade, timeAndSale});

    std::map<std::string, test::TapeRecord> records{};

    for (auto &record : test::parseTextTape(path)) {
        records[record.name] = std::move(record);
    }

    REQUIRE(records.count("Quote") == 1);
    REQUIRE(records.count("Trade") == 1);
    REQUIRE(records.count("TimeAndSale") == 1);

    return records;
}

} // namespace

TEST_CASE("A trade through exempt above ASCII is written as the Java API writes it; Order rejects such exchange code") {
    auto records = writeCharAboveAscii();

    CHECK(records["TimeAndSale"].fields["Flags"] == "50176");
    CHECK_THROWS(Order("CHAR").setExchangeCode('\xC4'));
}

TEST_CASE("An exchange code above ASCII is written as the Java API writes (char) 0xC4") {
    auto records = writeCharAboveAscii();

    CHECK(records["Quote"].fields["BidExchangeCode"] == "\\u00C4");
    CHECK(records["Trade"].fields["ExchangeCode"] == "\\u00C4");
    CHECK(records["TimeAndSale"].fields["ExchangeCode"] == "\\u00C4");
}

// The edge values that the Java API rejects on publishing (QD 3.355), so they are not in the golden tape.
TEST_CASE("The publisher rejects the events that the Java API rejects and accepts the others") {
    const auto order = [](const Side &side, double size, const std::string &marketMaker) {
        auto result = std::make_shared<Order>("REJECT");

        result->setSource(OrderSource::NTV);
        result->setOrderSide(side);
        result->setSize(size);
        result->setMarketMaker(marketMaker);

        return result;
    };
    const auto timeAndSale = [](const std::string &conditions) {
        auto result = std::make_shared<TimeAndSale>("REJECT");

        result->setExchangeSaleConditions(conditions);

        return result;
    };
    const auto publish = [](const std::shared_ptr<EventType> &event) {
        test::writeTextTape(test::temporaryFilePath("reject.txt"), {event});
    };

    SUBCASE("A non-empty order with the UNDEFINED side is rejected") {
        CHECK_THROWS(publish(order(Side::UNDEFINED, 1, "")));
    }

    SUBCASE("An empty order with the UNDEFINED side is accepted") {
        CHECK_NOTHROW(publish(order(Side::UNDEFINED, 0, "")));
    }

    SUBCASE("QD short strings of more than 8 characters are rejected") {
        CHECK_THROWS(publish(order(Side::BUY, 1, "ABCDEFGHI")));
        CHECK_THROWS(publish(timeAndSale("ABCDEFGHI")));
    }

    SUBCASE("QD short strings of 8 characters are accepted") {
        CHECK_NOTHROW(publish(order(Side::BUY, 1, "ABCDEFGH")));
        CHECK_NOTHROW(publish(timeAndSale("ABCDEFGH")));
    }

    SUBCASE("QD short strings accept the characters up to U+00FF and reject the others") {
        CHECK_NOTHROW(publish(order(Side::BUY, 1, "\x7F")));
        CHECK_NOTHROW(publish(order(Side::BUY, 1, "\xC2\x80")));    // U+0080
        CHECK_NOTHROW(publish(order(Side::BUY, 1, "\xC3\xBF")));    // U+00FF
        CHECK_THROWS(publish(order(Side::BUY, 1, "\xC4\x80")));     // U+0100
        CHECK_THROWS(publish(order(Side::BUY, 1, "\xE2\x82\xAC"))); // U+20AC
    }
}
