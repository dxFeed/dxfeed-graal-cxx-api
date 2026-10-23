// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"
#include "EventsJavaParity.hpp"

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
    return (std::isnan(actual) && std::isnan(expected)) || math::exactlyEquals(actual, expected);
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

namespace {

template <typename E>
const E &findByName(const std::span<const std::reference_wrapper<const E>> all, const std::string &name) {
    for (const auto &element : all) {
        if (element.get().getName() == name) {
            return element.get();
        }
    }

    throw std::runtime_error("No element " + name);
}

// toString() prints the times in the default time zone of the Java isolate without a zone suffix, the Java references
// are recorded with the zone UTC. The zone is read at the first use of TimeFormat in the process (TZ is not honored by
// the native SDK on Windows), so it is set when the test run starts, before any test formats a time. Not from a
// dynamic initializer: the library may be linked statically and not yet initialized there.
struct UtcTimeZoneListener : doctest::IReporter {
    explicit UtcTimeZoneListener(const doctest::ContextOptions &) {
    }

    void test_run_start() override {
        System::setProperty("user.timezone", "UTC");
    }

    void report_query(const doctest::QueryData &) override {
    }

    void test_run_end(const doctest::TestRunStats &) override {
    }

    void test_case_start(const doctest::TestCaseData &) override {
    }

    void test_case_reenter(const doctest::TestCaseData &) override {
    }

    void test_case_end(const doctest::CurrentTestCaseStats &) override {
    }

    void test_case_exception(const doctest::TestCaseException &) override {
    }

    void subcase_start(const doctest::SubcaseSignature &) override {
    }

    void subcase_end() override {
    }

    void log_assert(const doctest::AssertData &) override {
    }

    void log_message(const doctest::MessageData &) override {
    }

    void test_case_skipped(const doctest::TestCaseData &) override {
    }
};

DOCTEST_REGISTER_LISTENER("utc-time-zone", 0, UtcTimeZoneListener);

// The event with the values of the case that the Java probe recorded (tests/api/EventsJavaParity.hpp).
OrderImbalance makeFromJavaCase(const events_java_parity::OrderImbalanceCase &javaCase) {
    OrderImbalance imbalance{};

    if (javaCase.symbol != nullptr) {
        imbalance.setEventSymbol(javaCase.symbol);
    }

    imbalance.setEventTime(javaCase.eventTime);
    imbalance.setSource(OrderSource::valueOf(javaCase.source));
    imbalance.setTime(javaCase.time);
    imbalance.setSequence(javaCase.sequence);
    imbalance.setRefPrice(javaCase.refPrice);
    imbalance.setPairedSize(javaCase.pairedSize);
    imbalance.setImbalanceSize(javaCase.imbalanceSize);
    imbalance.setNearPrice(javaCase.nearPrice);
    imbalance.setFarPrice(javaCase.farPrice);
    imbalance.setImbalanceSide(findByName(ImbalanceSide::ALL, javaCase.imbalanceSide));
    imbalance.setAuctionType(findByName(AuctionType::ALL, javaCase.auctionType));

    return imbalance;
}

} // namespace

TEST_CASE("OrderImbalance has the flags of the Java class") {
    const auto &flags = events_java_parity::TYPE_FLAGS[0];

    REQUIRE(std::string(flags.name) == "OrderImbalance");
    CHECK(OrderImbalance::TYPE.isLasting() == flags.lasting);
    CHECK(OrderImbalance::TYPE.isIndexed() == flags.indexed);
    CHECK(OrderImbalance::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(OrderImbalance::TYPE.isMarket() == flags.market);
    CHECK(OrderImbalance::MAX_SEQUENCE == events_java_parity::ORDER_IMBALANCE_MAX_SEQUENCE);
}

TEST_CASE("OrderImbalance::toString() prints the fields as the Java API does") {
    // The zone must be UTC here (UtcTimeZoneListener), otherwise every comparison below fails on the time text.
    REQUIRE(TimeFormat::DEFAULT_WITH_MILLIS.format(1'700'000'000'123LL) == "20231114-221320.123");

    for (const auto &javaCase : events_java_parity::ORDER_IMBALANCE) {
        CAPTURE(javaCase.name);

        auto expected = std::string(javaCase.str);

        if (javaCase.symbol == nullptr) {
            // A null symbol is String::NUL (`<null>`) in the C++ API, Java prints `null`.
            const std::string javaNull = "OrderImbalance{null,";

            REQUIRE(expected.rfind(javaNull, 0) == 0);
            expected.replace(0, javaNull.size(), std::string("OrderImbalance{") + String::NUL + ",");
        }

        CHECK(makeFromJavaCase(javaCase).toString() == expected);
    }
}

namespace {

struct OrderImbalanceHubFixture : test::LocalHubFixture {
    OrderImbalanceHubFixture()
        : LocalHubFixture(
              std::unordered_map<std::string, std::string>{{DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true"}}) {
    }

    // Subscribes to the symbols, publishes the events and returns what the subscription received.
    std::vector<std::shared_ptr<OrderImbalance>>
    publishAndReceive(const std::vector<std::string> &symbols,
                      const std::vector<std::shared_ptr<OrderImbalance>> &published) {
        std::vector<std::shared_ptr<OrderImbalance>> received{};
        const auto subscription = DXFeedSubscription::create({OrderImbalance::TYPE});

        for (const auto &symbol : symbols) {
            subscription->addSymbols(symbol);
        }

        subscription->addEventListener<OrderImbalance>([&received](const auto &events) {
            for (auto &&event : events) {
                received.push_back(event);
            }
        });

        feed->attachSubscription(subscription);
        publishAndProcess(published);
        feed->detachSubscription(subscription);

        return received;
    }
};

std::shared_ptr<OrderImbalance> makeOnSource(const std::string &symbol, const std::string &source,
                                             std::int32_t sequence) {
    auto imbalance = std::make_shared<OrderImbalance>(makePopulated());

    imbalance->setEventSymbol(symbol);
    imbalance->setSource(OrderSource::valueOf(source));
    imbalance->setSequence(sequence);

    return imbalance;
}

} // namespace

TEST_CASE_FIXTURE(OrderImbalanceHubFixture, "OrderImbalance is published on NTV and received with every field") {
    auto published = std::make_shared<OrderImbalance>("AAPL");

    // A different value in every field.
    published->setEventTime(EVENT_TIME);
    published->setSource(OrderSource::NTV);
    published->setTime(TIME);
    published->setSequence(12345);
    published->setRefPrice(150.25);
    published->setPairedSize(1200.0);
    published->setImbalanceSize(300.5);
    published->setNearPrice(150.5);
    published->setFarPrice(149.75);
    published->setImbalanceSide(ImbalanceSide::BUY);
    published->setAuctionType(AuctionType::REOPENING);

    const auto received = publishAndReceive({"AAPL"}, {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "AAPL");
    CHECK(event.getEventTime() == EVENT_TIME);
    CHECK(event.getSource() == OrderSource::NTV);
    CHECK(event.getTimeSequence() == published->getTimeSequence());
    CHECK(event.getTime() == TIME); // the milliseconds survive
    CHECK(event.getSequence() == 12345);
    CHECK(event.getRefPrice() == 150.25);
    CHECK(event.getPairedSize() == 1200.0);
    CHECK(event.getImbalanceSize() == 300.5);
    CHECK(event.getNearPrice() == 150.5);
    CHECK(event.getFarPrice() == 149.75);
    CHECK(&event.getImbalanceSide() == &ImbalanceSide::BUY);
    CHECK(&event.getAuctionType() == &AuctionType::REOPENING);
    checkSame(event, *published);
}

TEST_CASE_FIXTURE(OrderImbalanceHubFixture,
                  "OrderImbalance keeps NaN fields, the maximum sequence and every enum value") {
    std::vector<std::shared_ptr<OrderImbalance>> published{};

    // The side and the type cycle through all their values, one event per symbol.
    for (std::size_t i = 0; i < ImbalanceSide::ALL.size(); i++) {
        auto imbalance = std::make_shared<OrderImbalance>("S" + std::to_string(i));

        imbalance->setSource(OrderSource::NUAM);
        imbalance->setTime(TIME + static_cast<std::int64_t>(i));
        imbalance->setSequence(i == 0 ? static_cast<std::int32_t>(OrderImbalance::MAX_SEQUENCE)
                                      : static_cast<std::int32_t>(i));
        imbalance->setRefPrice(i % 2 == 0 ? std::numeric_limits<double>::quiet_NaN() : 1.5 + static_cast<double>(i));
        imbalance->setImbalanceSide(ImbalanceSide::ALL[i]);
        imbalance->setAuctionType(AuctionType::ALL[i % AuctionType::ALL.size()]);
        published.push_back(imbalance);
    }

    std::vector<std::string> symbols{};

    for (const auto &event : published) {
        symbols.push_back(event->getEventSymbol());
    }

    const auto received = publishAndReceive(symbols, published);

    REQUIRE(received.size() == published.size());

    for (std::size_t i = 0; i < published.size(); i++) {
        CAPTURE(i);

        const auto found = std::find_if(received.begin(), received.end(), [&](const auto &event) {
            return event->getEventSymbol() == published[i]->getEventSymbol();
        });

        REQUIRE(found != received.end());
        checkSame(**found, *published[i]);
    }
}

TEST_CASE_FIXTURE(OrderImbalanceHubFixture,
                  "OrderImbalance of one symbol on several sources arrives with every source") {
    const auto ntv = makeOnSource("AAPL", "NTV", 1);
    const auto nuam = makeOnSource("AAPL", "NUAM", 2);
    const auto arca = makeOnSource("AAPL", "ARCA", 3);
    const auto defaultSource = makeOnSource("AAPL", "DEFAULT", 4);

    // The subscription is by the plain symbol, not by a wildcard: the Java API delivers all sources to it.
    const auto received = publishAndReceive({"AAPL"}, {ntv, nuam, arca, defaultSource});

    REQUIRE(received.size() == 4);

    for (const auto &expected : {ntv, nuam, arca, defaultSource}) {
        CAPTURE(expected->getSource().toString());

        const auto found = std::find_if(received.begin(), received.end(), [&](const auto &event) {
            return event->getSource() == expected->getSource();
        });

        REQUIRE(found != received.end());
        checkSame(**found, *expected);
    }
}
