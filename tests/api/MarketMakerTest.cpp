// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"
#include "../support/UtcTimeZone.hpp"
#include "EventsJavaParity.hpp"

using namespace dxfcpp;
using namespace std::literals;

namespace {

// Whole seconds: the default scheme transfers the bid and ask times in seconds.
constexpr std::int64_t BID_TIME = 1'701'703'300'000LL;
constexpr std::int64_t ASK_TIME = 1'701'703'301'000LL;
constexpr std::int64_t EVENT_TIME = 1'701'703'226'537LL;

const events_java_parity::TypeFlags &flagsOf(const std::string &name) {
    for (const auto &flags : events_java_parity::TYPE_FLAGS) {
        if (name == flags.name) {
            return flags;
        }
    }

    throw std::runtime_error("No flags of " + name);
}

const events_java_parity::MarketMakerCase &caseNamed(const std::string &name) {
    for (const auto &javaCase : events_java_parity::MARKET_MAKER) {
        if (name == javaCase.name) {
            return javaCase;
        }
    }

    throw std::runtime_error("No case " + name);
}

// Two doubles are the same value: both NaN or equal (without == to keep -Wfloat-equal quiet).
bool sameDouble(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }

    return !(a < b) && !(b < a);
}

// The event with the values of the case that the Java probe recorded (tests/api/EventsJavaParity.hpp). The exchange
// code and the market maker id come from the index that Java made of them.
MarketMaker makeFromJavaCase(const events_java_parity::MarketMakerCase &javaCase) {
    MarketMaker marketMaker{};

    if (javaCase.symbol != nullptr) {
        marketMaker.setEventSymbol(javaCase.symbol);
    }

    marketMaker.setEventTime(javaCase.eventTime);
    marketMaker.setEventFlags(javaCase.eventFlags);
    marketMaker.setIndex(javaCase.index);
    marketMaker.setBidTime(javaCase.bidTime);
    marketMaker.setBidPrice(javaCase.bidPrice);
    marketMaker.setBidSize(javaCase.bidSize);
    marketMaker.setBidCount(javaCase.bidCount);
    marketMaker.setAskTime(javaCase.askTime);
    marketMaker.setAskPrice(javaCase.askPrice);
    marketMaker.setAskSize(javaCase.askSize);
    marketMaker.setAskCount(javaCase.askCount);

    return marketMaker;
}

// A different value in every field.
std::shared_ptr<MarketMaker> makePopulated(const std::string &symbol, char exchangeCode, const std::string &id) {
    auto marketMaker = std::make_shared<MarketMaker>(symbol);

    marketMaker->setEventTime(EVENT_TIME);
    marketMaker->setExchangeCode(exchangeCode);
    marketMaker->setMarketMaker(id);
    marketMaker->setBidTime(BID_TIME);
    marketMaker->setBidPrice(150.25);
    marketMaker->setBidSize(100.5);
    marketMaker->setBidCount(7);
    marketMaker->setAskTime(ASK_TIME);
    marketMaker->setAskPrice(150.5);
    marketMaker->setAskSize(200.75);
    marketMaker->setAskCount(9);

    return marketMaker;
}

} // namespace

TEST_CASE("MarketMaker has the flags of the Java class") {
    const auto flags = flagsOf("MarketMaker");

    CHECK(MarketMaker::TYPE.isLasting() == flags.lasting);
    CHECK(MarketMaker::TYPE.isIndexed() == flags.indexed);
    CHECK(MarketMaker::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(MarketMaker::TYPE.isMarket() == flags.market);
    CHECK(&EventTypeEnum::ALL_BY_NAME.at("MARKET_MAKER").get() == &EventTypeEnum::MARKET_MAKER);
    CHECK(&EventTypeEnum::ALL_BY_CLASS_NAME.at("MarketMaker").get() == &EventTypeEnum::MARKET_MAKER);
}

TEST_CASE("MarketMaker::toString() prints the fields as the Java API does") {
    // The zone must be UTC here (UtcTimeZoneListener), otherwise every comparison below fails on the time text.
    REQUIRE(TimeFormat::DEFAULT_WITH_MILLIS.format(1'700'000'000'123LL) == "20231114-221320.123");

    for (const auto &javaCase : events_java_parity::MARKET_MAKER) {
        CAPTURE(javaCase.name);

        auto expected = std::string(javaCase.str);

        if (javaCase.symbol == nullptr) {
            // A null symbol is String::NUL (`<null>`) in the C++ API, Java prints `null`.
            const auto javaNull = "MarketMaker{null,"s;

            REQUIRE(expected.rfind(javaNull, 0) == 0);
            expected.replace(0, javaNull.size(), "MarketMaker{"s + String::NUL + ",");
        }

        CHECK(makeFromJavaCase(javaCase).toString() == expected);
    }
}

TEST_CASE("MarketMaker makes the index of the exchange code and the market maker id as the Java API does") {
    for (const auto &javaCase : events_java_parity::MARKET_MAKER) {
        CAPTURE(javaCase.name);

        MarketMaker marketMaker{};

        marketMaker.setExchangeCode(static_cast<std::int16_t>(javaCase.exchangeCode));

        if (javaCase.marketMaker != nullptr) {
            marketMaker.setMarketMaker(javaCase.marketMaker);
        }

        CHECK(marketMaker.getIndex() == javaCase.index);
        CHECK(marketMaker.getExchangeCode() == static_cast<std::int16_t>(javaCase.exchangeCode));
    }
}

TEST_CASE("MarketMaker keeps the exchange code and the market maker id apart in the index") {
    MarketMaker marketMaker{};

    marketMaker.setExchangeCode('Q');
    marketMaker.setMarketMaker("ABCD");

    CHECK(marketMaker.getIndex() == 0x51'4142'4344LL);
    CHECK(marketMaker.getExchangeCode() == 'Q');
    CHECK(marketMaker.getExchangeCodeString() == "Q");
    CHECK(marketMaker.getMarketMaker() == "ABCD");

    marketMaker.setExchangeCode('Z');

    CHECK(marketMaker.getMarketMaker() == "ABCD");

    marketMaker.setMarketMaker("XY");

    CHECK(marketMaker.getExchangeCode() == 'Z');
    CHECK(marketMaker.getIndex() == 0x5A'0000'5859LL);

    marketMaker.setIndex(0x51'4142'4344LL);

    CHECK(marketMaker.getExchangeCode() == 'Q');
    CHECK(marketMaker.getMarketMaker() == "ABCD");
}

TEST_CASE("MarketMaker market maker id is a QD short string") {
    MarketMaker marketMaker{};

    SUBCASE("Unset and empty ids are null") {
        CHECK(marketMaker.getMarketMakerOpt() == std::nullopt);
        CHECK(marketMaker.getMarketMaker() == String::NUL);

        marketMaker.setMarketMaker("NSDQ");
        marketMaker.setMarketMaker("");

        CHECK(marketMaker.getMarketMakerOpt() == std::nullopt);
        CHECK(marketMaker.getIndex() == 0);
    }

    SUBCASE("The index keeps the last 4 characters") {
        marketMaker.setMarketMaker("ABCDEF");

        CHECK(marketMaker.getMarketMaker() == "CDEF");

        marketMaker.setMarketMaker("ABCDEFGH");

        CHECK(marketMaker.getMarketMaker() == "EFGH");
    }

    SUBCASE("Zero characters are skipped") {
        marketMaker.setMarketMaker(std::string("\0A\0B", 4));

        CHECK(marketMaker.getMarketMaker() == "AB");
        CHECK(marketMaker.getIndex() == 0x4142);
    }

    SUBCASE("Latin-1 characters are UTF-8 in the C++ API") {
        marketMaker.setMarketMaker("M\xC3\x9C\xC3\xBF"); // M U+00DC U+00FF

        CHECK(marketMaker.getIndex() == 0x4D'DCFFLL);
        CHECK(marketMaker.getMarketMaker() == "M\xC3\x9C\xC3\xBF");
    }

    SUBCASE("A long string, a character above U+00FF and invalid UTF-8 are rejected and change nothing") {
        marketMaker.setMarketMaker("NSDQ");

        CHECK_THROWS_AS(marketMaker.setMarketMaker("ABCDEFGHI"), InvalidArgumentException);
        CHECK_THROWS_AS(marketMaker.setMarketMaker("A\xE2\x82\xAC"), InvalidArgumentException); // A U+20AC
        CHECK_THROWS_AS(marketMaker.setMarketMaker("A\xC3"), InvalidArgumentException);
        CHECK_THROWS_AS(marketMaker.withMarketMaker("\xFF"), InvalidArgumentException);

        CHECK(marketMaker.getMarketMaker() == "NSDQ");
    }
}

TEST_CASE("MarketMaker exchange codes above U+7FFF are kept and printed as Java does") {
    MarketMaker marketMaker{};

    marketMaker.setExchangeCode(static_cast<std::int16_t>(0xFFFF));
    marketMaker.setMarketMaker("Z");

    CHECK(marketMaker.getIndex() == 0xFFFF'0000'005ALL);
    CHECK(marketMaker.getExchangeCode() == static_cast<std::int16_t>(0xFFFF));
    CHECK(marketMaker.toString().find("exchange=\\uffff,") != std::string::npos);
}

TEST_CASE("MarketMaker rejects a negative index") {
    MarketMaker marketMaker{};

    marketMaker.setIndex(42);

    CHECK_THROWS_AS(marketMaker.setIndex(-1), InvalidArgumentException);
    CHECK_THROWS_AS(marketMaker.withIndex(std::numeric_limits<std::int64_t>::min()), InvalidArgumentException);
    CHECK(marketMaker.getIndex() == 42);
}

TEST_CASE("MarketMaker time is the later of the bid and ask times") {
    MarketMaker marketMaker{};

    marketMaker.setBidTime(2'000);
    marketMaker.setAskTime(1'000);

    CHECK(marketMaker.getTime() == 2'000);

    marketMaker.setAskTime(3'000);

    CHECK(marketMaker.getTime() == 3'000);
    CHECK(&marketMaker.getSource() == &IndexedEventSource::DEFAULT);
}

TEST_CASE("MarketMaker is converted to the Graal structure and back with every field") {
    const auto published = makePopulated("AAPL", 'Q', "NSDQ");

    published->setEventFlags(static_cast<std::int32_t>(EventFlag::SNAPSHOT_BEGIN.getFlag()));

    auto *graal = published->toGraal();

    REQUIRE(graal != nullptr);

    const auto restored = MarketMaker::fromGraal(graal);

    MarketMaker::freeGraal(graal);

    CHECK(restored->toString() == published->toString());
    CHECK(restored->getEventFlags() == static_cast<std::int32_t>(EventFlag::SNAPSHOT_BEGIN.getFlag()));
    CHECK_THROWS_AS(MarketMaker::fromGraal(nullptr), InvalidArgumentException);

    // A structure of another event class is rejected.
    Quote quote("AAPL");
    auto *graalQuote = quote.toGraal();

    CHECK_THROWS_AS(MarketMaker::fromGraal(graalQuote), InvalidArgumentException);
    CHECK_THROWS_AS(MarketMaker::freeGraal(graalQuote), InvalidArgumentException);

    Quote::freeGraal(graalQuote);
}

TEST_CASE("assign() copies every field of MarketMaker") {
    const auto source = std::make_shared<MarketMaker>(makeFromJavaCase(caseNamed("full")));
    const auto target = std::make_shared<MarketMaker>();

    target->assign(source);

    CHECK(target->toString() == source->toString());
    CHECK(target->getEventSymbol() == "AAPL");
    CHECK(target->getEventFlags() == 3);
    CHECK(target->getMarketMaker() == "NSDQ");
    CHECK(target->getExchangeCode() == 'Q');
    CHECK(target->getAskCount() == 9);
}

namespace {

struct MarketMakerHubFixture : test::LocalHubFixture {
    MarketMakerHubFixture()
        : LocalHubFixture(
              std::unordered_map<std::string, std::string>{{DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true"}}) {
    }

    std::vector<std::shared_ptr<MarketMaker>>
    publishAndReceive(const std::string &symbol, const std::vector<std::shared_ptr<MarketMaker>> &events) {
        std::vector<std::shared_ptr<MarketMaker>> received{};
        const auto subscription = DXFeedSubscription::create({MarketMaker::TYPE});

        subscription->addSymbols(symbol);
        subscription->addEventListener<MarketMaker>([&received](const auto &newEvents) {
            for (auto &&event : newEvents) {
                received.push_back(event);
            }
        });

        feed->attachSubscription(subscription);
        publishAndProcess(events);
        feed->detachSubscription(subscription);

        return received;
    }
};

} // namespace

TEST_CASE_FIXTURE(MarketMakerHubFixture, "MarketMaker is published and received with every field") {
    // Two market makers on one exchange: the index tells them apart.
    const auto nsdq = makePopulated("AAPL", 'Q', "NSDQ");
    const auto arcx = makePopulated("AAPL", 'Q', "ARCX");

    arcx->setBidPrice(150.0);
    arcx->setAskCount(11);

    const auto received = publishAndReceive("AAPL", {nsdq, arcx});

    REQUIRE(received.size() == 2);

    const auto &first = received[0]->getMarketMaker() == "NSDQ" ? *received[0] : *received[1];
    const auto &second = received[0]->getMarketMaker() == "NSDQ" ? *received[1] : *received[0];

    CHECK(first.getEventSymbol() == "AAPL");
    CHECK(first.getEventTime() == EVENT_TIME);
    CHECK(first.getIndex() == nsdq->getIndex());
    CHECK(first.getExchangeCode() == 'Q');
    CHECK(first.getMarketMaker() == "NSDQ");
    CHECK(first.getBidTime() == BID_TIME);
    CHECK(sameDouble(first.getBidPrice(), 150.25));
    CHECK(sameDouble(first.getBidSize(), 100.5));
    CHECK(first.getBidCount() == 7);
    CHECK(first.getAskTime() == ASK_TIME);
    CHECK(sameDouble(first.getAskPrice(), 150.5));
    CHECK(sameDouble(first.getAskSize(), 200.75));
    CHECK(first.getAskCount() == 9);
    CHECK(first.getTime() == ASK_TIME);

    CHECK(second.getMarketMaker() == "ARCX");
    CHECK(second.getIndex() == arcx->getIndex());
    CHECK(sameDouble(second.getBidPrice(), 150.0));
    CHECK(second.getAskCount() == 11);
}

TEST_CASE_FIXTURE(MarketMakerHubFixture, "MarketMaker with a null id and NaN fields keeps them") {
    auto published = std::make_shared<MarketMaker>("MSFT");

    published->setExchangeCode(static_cast<std::int16_t>(0xDC));
    published->setEventTime(EVENT_TIME);

    const auto received = publishAndReceive("MSFT", {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getExchangeCode() == 0xDC);
    CHECK(event.getMarketMakerOpt() == std::nullopt);
    CHECK(std::isnan(event.getBidPrice()));
    CHECK(std::isnan(event.getBidSize()));
    CHECK(std::isnan(event.getAskPrice()));
    CHECK(std::isnan(event.getAskSize()));
    CHECK(event.getBidTime() == 0);
    CHECK(event.getAskCount() == 0);
}
