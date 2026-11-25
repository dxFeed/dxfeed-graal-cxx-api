// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cstdint>
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

constexpr std::int64_t EVENT_TIME = 1'701'703'226'537LL;
constexpr std::int64_t TIME = 1'701'703'300'123LL;

// U+00DC U+20AC U+1F600: two-, three- and four-byte UTF-8 sequences.
constexpr auto NON_ASCII_TEXT = "\xC3\x9C\xE2\x82\xAC\xF0\x9F\x98\x80";

const events_java_parity::TypeFlags &flagsOf(const std::string &name) {
    for (const auto &flags : events_java_parity::TYPE_FLAGS) {
        if (name == flags.name) {
            return flags;
        }
    }

    throw std::runtime_error("No flags of " + name);
}

const events_java_parity::TextConfigurationCase &caseNamed(const std::string &name) {
    for (const auto &javaCase : events_java_parity::TEXT_CONFIGURATION) {
        if (name == javaCase.name) {
            return javaCase;
        }
    }

    throw std::runtime_error("No case " + name);
}

// The event with the values of the case that the Java probe recorded (tests/api/EventsJavaParity.hpp).
TextConfiguration makeFromJavaCase(const events_java_parity::TextConfigurationCase &javaCase) {
    TextConfiguration configuration{};

    if (javaCase.symbol != nullptr) {
        configuration.setEventSymbol(javaCase.symbol);
    }

    configuration.setEventTime(javaCase.eventTime);
    configuration.setTime(javaCase.time);
    configuration.setSequence(javaCase.sequence);
    configuration.setVersion(javaCase.version);

    if (javaCase.text != nullptr) {
        configuration.setText(javaCase.text);
    }

    return configuration;
}

} // namespace

TEST_CASE("TextConfiguration has the flags of the Java class") {
    const auto flags = flagsOf("TextConfiguration");

    CHECK(TextConfiguration::TYPE.isLasting() == flags.lasting);
    CHECK(TextConfiguration::TYPE.isIndexed() == flags.indexed);
    CHECK(TextConfiguration::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(TextConfiguration::TYPE.isMarket() == flags.market);
    CHECK(&EventTypeEnum::ALL_BY_NAME.at("TEXT_CONFIGURATION").get() == &EventTypeEnum::TEXT_CONFIGURATION);
    CHECK(&EventTypeEnum::ALL_BY_CLASS_NAME.at("TextConfiguration").get() == &EventTypeEnum::TEXT_CONFIGURATION);
    CHECK(TextConfiguration::MAX_SEQUENCE == events_java_parity::TEXT_CONFIGURATION_MAX_SEQUENCE);
}

TEST_CASE("TextConfiguration::toString() prints the fields as the Java API does") {
    // The zone must be UTC here (UtcTimeZoneListener), otherwise every comparison below fails on the time text.
    REQUIRE(TimeFormat::DEFAULT_WITH_MILLIS.format(1'700'000'000'123LL) == "20231114-221320.123");

    for (const auto &javaCase : events_java_parity::TEXT_CONFIGURATION) {
        CAPTURE(javaCase.name);

        auto expected = std::string(javaCase.str);

        if (javaCase.symbol == nullptr) {
            // A null symbol is String::NUL (`<null>`) in the C++ API, Java prints `null`.
            const auto javaNull = "TextConfiguration{null,"s;

            REQUIRE(expected.rfind(javaNull, 0) == 0);
            expected.replace(0, javaNull.size(), "TextConfiguration{"s + String::NUL + ",");
        }

        CHECK(makeFromJavaCase(javaCase).toString() == expected);
    }
}

TEST_CASE("TextConfiguration constructors and the null text") {
    const TextConfiguration empty{};

    CHECK(empty.getEventSymbolOpt() == std::nullopt);
    CHECK(empty.getTextOpt() == std::nullopt);
    CHECK(empty.getText() == String::NUL);
    CHECK(empty.getVersion() == 0);

    const TextConfiguration withText("AAPL", "text");

    CHECK(withText.getEventSymbol() == "AAPL");
    CHECK(withText.getText() == "text");
    CHECK(withText.getVersion() == 0);

    const TextConfiguration withVersion("AAPL", "", 5);

    CHECK(withVersion.getTextOpt() == std::optional<std::string>{""});
    CHECK(withVersion.getVersion() == 5);
}

TEST_CASE("TextConfiguration keeps time and sequence apart") {
    TextConfiguration configuration("AAPL");

    configuration.setSequence(42);
    configuration.setTime(TIME);

    CHECK(configuration.getTime() == TIME);
    CHECK(configuration.getSequence() == 42);

    configuration.withSequence(static_cast<std::int32_t>(TextConfiguration::MAX_SEQUENCE)).withTime(TIME + 1);

    CHECK(configuration.getTime() == TIME + 1);
    CHECK(configuration.getSequence() == static_cast<std::int32_t>(TextConfiguration::MAX_SEQUENCE));

    const auto timeSequence = configuration.getTimeSequence();

    CHECK_THROWS_AS(configuration.setSequence(-1), InvalidArgumentException);
    CHECK_THROWS_AS(configuration.setSequence(static_cast<std::int32_t>(TextConfiguration::MAX_SEQUENCE) + 1),
                    InvalidArgumentException);
    CHECK(configuration.getTimeSequence() == timeSequence);
}

TEST_CASE("TextConfiguration is converted to the Graal structure and back with every field") {
    SUBCASE("Set fields") {
        const auto published = std::make_shared<TextConfiguration>(makeFromJavaCase(caseNamed("unicode")));
        auto *graal = published->toGraal();

        REQUIRE(graal != nullptr);

        const auto restored = TextConfiguration::fromGraal(graal);

        TextConfiguration::freeGraal(graal);

        CHECK(restored->toString() == published->toString());
        CHECK(restored->getText() == NON_ASCII_TEXT);
    }

    SUBCASE("Null symbol and text") {
        const TextConfiguration published{};
        auto *graal = published.toGraal();
        const auto restored = TextConfiguration::fromGraal(graal);

        TextConfiguration::freeGraal(graal);

        CHECK(restored->getEventSymbolOpt() == std::nullopt);
        CHECK(restored->getTextOpt() == std::nullopt);
    }

    SUBCASE("Another event class") {
        CHECK_THROWS_AS(TextConfiguration::fromGraal(nullptr), InvalidArgumentException);

        TextMessage message("AAPL", "text");
        auto *graalMessage = message.toGraal();

        CHECK_THROWS_AS(TextConfiguration::fromGraal(graalMessage), InvalidArgumentException);
        CHECK_THROWS_AS(TextConfiguration::freeGraal(graalMessage), InvalidArgumentException);

        TextMessage::freeGraal(graalMessage);
    }
}

TEST_CASE("assign() copies every field of TextConfiguration") {
    const auto source = std::make_shared<TextConfiguration>(makeFromJavaCase(caseNamed("full")));
    const auto target = std::make_shared<TextConfiguration>("OTHER", "other", 1);

    target->assign(source);

    CHECK(target->toString() == source->toString());
    CHECK(target->getEventSymbol() == "AAPL");
    CHECK(target->getVersion() == 7);
    CHECK(target->getSequence() == 42);
    CHECK(target->getText() == R"({"key":"value"})");
}

namespace {

struct TextConfigurationHubFixture : test::LocalHubFixture {
    TextConfigurationHubFixture()
        : LocalHubFixture(
              std::unordered_map<std::string, std::string>{{DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true"}}) {
    }

    std::vector<std::shared_ptr<TextConfiguration>>
    publishAndReceive(const std::string &symbol, const std::vector<std::shared_ptr<TextConfiguration>> &events) {
        std::vector<std::shared_ptr<TextConfiguration>> received{};
        const auto subscription = DXFeedSubscription::create({TextConfiguration::TYPE});

        subscription->addSymbols(symbol);
        subscription->addEventListener<TextConfiguration>([&received](const auto &newEvents) {
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

TEST_CASE_FIXTURE(TextConfigurationHubFixture, "TextConfiguration is published and received with every field") {
    auto published = std::make_shared<TextConfiguration>("AAPL", NON_ASCII_TEXT, 7);

    published->setEventTime(EVENT_TIME);
    published->setTime(TIME);
    published->setSequence(42);

    const auto received = publishAndReceive("AAPL", {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "AAPL");
    CHECK(event.getEventTime() == EVENT_TIME);
    CHECK(event.getTime() == TIME);
    CHECK(event.getSequence() == 42);
    CHECK(event.getVersion() == 7);
    CHECK(event.getText() == NON_ASCII_TEXT);
}

TEST_CASE_FIXTURE(TextConfigurationHubFixture, "TextConfiguration with a null text keeps it") {
    auto published = std::make_shared<TextConfiguration>("MSFT");

    // A version below the last one (0 for a new symbol) is dropped under the TICKER contract, as in Java.
    published->setVersion(2);

    const auto received = publishAndReceive("MSFT", {published});

    REQUIRE(received.size() == 1);
    CHECK(received[0]->getTextOpt() == std::nullopt);
    CHECK(received[0]->getVersion() == 2);
}
