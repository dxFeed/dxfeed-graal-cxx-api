// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

namespace {

constexpr std::int64_t EVENT_TIME = 1'701'703'226'537LL;
constexpr std::int64_t TIME = 1'701'703'300'123LL;

OrderImbalance makePopulated() {
    OrderImbalance imbalance("AAPL");

    imbalance.setEventTime(EVENT_TIME);
    imbalance.setSource(OrderSource::valueOf("NUAM"));
    imbalance.setTime(TIME);
    imbalance.setSequence(12345);
    imbalance.setRefPrice(100.5);
    imbalance.setPairedSize(1000.0);
    imbalance.setImbalanceSize(200.25);
    imbalance.setNearPrice(100.75);
    imbalance.setFarPrice(99.5);
    imbalance.setImbalanceSide(ImbalanceSide::SELL);
    imbalance.setAuctionType(AuctionType::CLOSING);

    return imbalance;
}

// The same value or both are NaN.
bool sameDouble(double actual, double expected) {
    return (std::isnan(actual) && std::isnan(expected)) || actual == expected;
}

void checkSame(const OrderImbalance &actual, const OrderImbalance &expected) {
    CHECK(actual.getEventSymbol() == expected.getEventSymbol());
    CHECK(actual.getEventTime() == expected.getEventTime());
    CHECK(actual.getSource() == expected.getSource());
    CHECK(actual.getTimeSequence() == expected.getTimeSequence());
    CHECK(actual.getTime() == expected.getTime());
    CHECK(actual.getSequence() == expected.getSequence());
    CHECK(sameDouble(actual.getRefPrice(), expected.getRefPrice()));
    CHECK(sameDouble(actual.getPairedSize(), expected.getPairedSize()));
    CHECK(sameDouble(actual.getImbalanceSize(), expected.getImbalanceSize()));
    CHECK(sameDouble(actual.getNearPrice(), expected.getNearPrice()));
    CHECK(sameDouble(actual.getFarPrice(), expected.getFarPrice()));
    CHECK(&actual.getImbalanceSide() == &expected.getImbalanceSide());
    CHECK(&actual.getAuctionType() == &expected.getAuctionType());
}

} // namespace

TEST_CASE("OrderImbalance has the Java default values") {
    const OrderImbalance imbalance{};

    CHECK(imbalance.getEventSymbol() == String::NUL);
    CHECK(imbalance.getEventTime() == 0);
    CHECK(imbalance.getSource() == OrderSource::DEFAULT);
    CHECK(imbalance.getTimeSequence() == 0);
    CHECK(imbalance.getTime() == 0);
    CHECK(imbalance.getSequence() == 0);
    CHECK(std::isnan(imbalance.getRefPrice()));
    CHECK(std::isnan(imbalance.getPairedSize()));
    CHECK(std::isnan(imbalance.getImbalanceSize()));
    CHECK(std::isnan(imbalance.getNearPrice()));
    CHECK(std::isnan(imbalance.getFarPrice()));
    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::UNDEFINED);
    CHECK(&imbalance.getAuctionType() == &AuctionType::UNDEFINED);

    CHECK(OrderImbalance("AAPL").getEventSymbol() == "AAPL");
}

TEST_CASE("OrderImbalance is a market event that is neither lasting nor indexed nor time series") {
    CHECK(OrderImbalance::TYPE == EventTypeEnum::ORDER_IMBALANCE);
    CHECK(OrderImbalance::TYPE.isMarket());
    CHECK_FALSE(OrderImbalance::TYPE.isLasting());
    CHECK_FALSE(OrderImbalance::TYPE.isIndexed());
    CHECK_FALSE(OrderImbalance::TYPE.isTimeSeries());
    CHECK(&EventTypeEnum::ALL_BY_NAME.at("ORDER_IMBALANCE").get() == &EventTypeEnum::ORDER_IMBALANCE);
    CHECK(&EventTypeEnum::ALL_BY_CLASS_NAME.at("OrderImbalance").get() == &EventTypeEnum::ORDER_IMBALANCE);
    CHECK(OrderImbalance("AAPL").is<MarketEvent>());
    CHECK_FALSE(OrderImbalance("AAPL").is<IndexedEvent>());
}

TEST_CASE("OrderImbalance imbalance side and auction type share the flags without affecting each other") {
    OrderImbalance imbalance{};

    imbalance.setImbalanceSide(ImbalanceSide::SELL);

    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::SELL);
    CHECK(&imbalance.getAuctionType() == &AuctionType::UNDEFINED);

    imbalance.setAuctionType(AuctionType::OTHER);

    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::SELL);
    CHECK(&imbalance.getAuctionType() == &AuctionType::OTHER);

    imbalance.setImbalanceSide(ImbalanceSide::PAUSED);

    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::PAUSED);
    CHECK(&imbalance.getAuctionType() == &AuctionType::OTHER);

    imbalance.setAuctionType(AuctionType::UNDEFINED);

    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::PAUSED);
    CHECK(&imbalance.getAuctionType() == &AuctionType::UNDEFINED);

    imbalance.setImbalanceSide(ImbalanceSide::UNDEFINED);

    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::UNDEFINED);
}

TEST_CASE("OrderImbalance time, sequence and source are stored independently") {
    OrderImbalance imbalance{};

    imbalance.setTime(TIME);
    imbalance.setSequence(12345);

    CHECK(imbalance.getTime() == TIME);
    CHECK(imbalance.getSequence() == 12345);

    imbalance.setTime(TIME + 1000);

    CHECK(imbalance.getTime() == TIME + 1000);
    CHECK(imbalance.getSequence() == 12345);

    imbalance.setSequence(static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE));

    CHECK(imbalance.getTime() == TIME + 1000);
    CHECK(imbalance.getSequence() == static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE));

    imbalance.setSource(OrderSource::valueOf("NUAM"));

    CHECK(imbalance.getSource() == OrderSource::valueOf("NUAM"));
    CHECK(imbalance.getTime() == TIME + 1000);
    CHECK(imbalance.getSequence() == static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE));
}

TEST_CASE("OrderImbalance::setSequence() rejects the values out of [0, MAX_SEQUENCE]") {
    OrderImbalance imbalance{};

    imbalance.setSequence(7);

    CHECK_THROWS_AS(imbalance.setSequence(-1), InvalidArgumentException);
    CHECK_THROWS_AS(imbalance.setSequence(static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE + 1U)),
                    InvalidArgumentException);
    CHECK_THROWS_AS(imbalance.withSequence(-1), InvalidArgumentException);
    CHECK_THROWS_AS(imbalance.withSequence(static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE + 1U)),
                    InvalidArgumentException);

    // The rejected values change nothing.
    CHECK(imbalance.getSequence() == 7);
    CHECK_NOTHROW(imbalance.setSequence(0));
    CHECK_NOTHROW(imbalance.setSequence(static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE)));
}

TEST_CASE("OrderImbalance withXxx() methods return the same event") {
    OrderImbalance imbalance{};

    auto &result = imbalance.withEventSymbol("AAPL")
                       .withEventTime(EVENT_TIME)
                       .withSource(OrderSource::valueOf("NUAM"))
                       .withTimeSequence(0)
                       .withTime(TIME)
                       .withSequence(5)
                       .withRefPrice(1.5)
                       .withPairedSize(2.5)
                       .withImbalanceSize(3.5)
                       .withNearPrice(4.5)
                       .withFarPrice(5.5)
                       .withImbalanceSide(ImbalanceSide::BUY)
                       .withAuctionType(AuctionType::OPENING);

    CHECK(&result == &imbalance);
    CHECK(imbalance.getEventSymbol() == "AAPL");
    CHECK(imbalance.getEventTime() == EVENT_TIME);
    CHECK(imbalance.getSource() == OrderSource::valueOf("NUAM"));
    CHECK(imbalance.getTime() == TIME);
    CHECK(imbalance.getSequence() == 5);
    CHECK(imbalance.getRefPrice() == 1.5);
    CHECK(imbalance.getPairedSize() == 2.5);
    CHECK(imbalance.getImbalanceSize() == 3.5);
    CHECK(imbalance.getNearPrice() == 4.5);
    CHECK(imbalance.getFarPrice() == 5.5);
    CHECK(&imbalance.getImbalanceSide() == &ImbalanceSide::BUY);
    CHECK(&imbalance.getAuctionType() == &AuctionType::OPENING);
}

TEST_CASE("OrderImbalance Graal conversion should keep every field") {
    const auto original = makePopulated();

    const auto graalNative = original.toGraal();

    REQUIRE(graalNative != nullptr);

    const auto restored = OrderImbalance::fromGraal(graalNative);

    checkSame(*restored, original);
    CHECK(restored->getEventSymbol() == "AAPL");
    CHECK(restored->getEventTime() == EVENT_TIME);
    CHECK(restored->getSource() == OrderSource::valueOf("NUAM"));
    CHECK(restored->getTime() == TIME);
    CHECK(restored->getSequence() == 12345);
    CHECK(restored->getRefPrice() == 100.5);
    CHECK(restored->getPairedSize() == 1000.0);
    CHECK(restored->getImbalanceSize() == 200.25);
    CHECK(restored->getNearPrice() == 100.75);
    CHECK(restored->getFarPrice() == 99.5);
    CHECK(&restored->getImbalanceSide() == &ImbalanceSide::SELL);
    CHECK(&restored->getAuctionType() == &AuctionType::CLOSING);
    CHECK(restored->toString() == original.toString());

    OrderImbalance::freeGraal(graalNative);
}

TEST_CASE("OrderImbalance Graal conversion should keep the default event") {
    const OrderImbalance original{};

    const auto graalNative = original.toGraal();
    const auto restored = OrderImbalance::fromGraal(graalNative);

    checkSame(*restored, original);
    CHECK(std::isnan(restored->getRefPrice()));
    CHECK(std::isnan(restored->getFarPrice()));

    OrderImbalance::freeGraal(graalNative);
}

TEST_CASE("OrderImbalance::fromGraal() and freeGraal() reject a structure of another event class") {
    // The structure of a Quote: the pointer of another event class.
    const Quote quote("AAPL");
    const auto graalQuote = quote.toGraal();

    const auto check = [](auto &&call) {
        std::string message{};

        try {
            call();
        } catch (const InvalidArgumentException &e) {
            message = e.what();
        }

        CAPTURE(message);

        CHECK_FALSE(message.empty());
        // The message names the expected event class and the actual one, none of them is UNKNOWN.
        CHECK(message.find("DXFG_EVENT_ORDER_IMBALANCE") != std::string::npos);
        CHECK(message.find("DXFG_EVENT_QUOTE") != std::string::npos);
        CHECK(message.find("UNKNOWN") == std::string::npos);
    };

    check([&] {
        OrderImbalance::fromGraal(graalQuote);
    });
    check([&] {
        OrderImbalance::freeGraal(graalQuote);
    });

    Quote::freeGraal(graalQuote);

    CHECK_THROWS_AS(OrderImbalance::fromGraal(nullptr), InvalidArgumentException);
    CHECK_NOTHROW(OrderImbalance::freeGraal(nullptr));
}

TEST_CASE("OrderImbalance::assign() copies every field") {
    const auto source = std::make_shared<OrderImbalance>(makePopulated());
    const auto target = std::make_shared<OrderImbalance>();

    target->assign(source);

    checkSame(*target, *source);
    CHECK(&target->getImbalanceSide() == &ImbalanceSide::SELL);
    CHECK(&target->getAuctionType() == &AuctionType::CLOSING);
}

TEST_CASE("OrderImbalance::toString() prints the fields as Java does") {
    const auto imbalance = makePopulated();

    const auto expected =
        "OrderImbalance{AAPL, eventTime=" + TimeFormat::DEFAULT_WITH_MILLIS.format(EVENT_TIME) +
        ", source=NUAM, time=" + TimeFormat::DEFAULT_WITH_MILLIS.format(TIME) +
        ", sequence=12345, refPrice=100.5, pairedSize=1000.0, imbalanceSize=200.25, nearPrice=100.75, "
        "farPrice=99.5, imbalanceSide=SELL, auctionType=CLOSING}";

    CHECK(imbalance.toString() == expected);

    const auto defaults = OrderImbalance("AAPL").toString();

    CHECK(defaults == "OrderImbalance{AAPL, eventTime=" + TimeFormat::DEFAULT_WITH_MILLIS.format(0) +
                          ", source=DEFAULT, time=" + TimeFormat::DEFAULT_WITH_MILLIS.format(0) +
                          ", sequence=0, refPrice=NaN, pairedSize=NaN, imbalanceSize=NaN, nearPrice=NaN, "
                          "farPrice=NaN, imbalanceSide=UNDEFINED, auctionType=UNDEFINED}");
}
