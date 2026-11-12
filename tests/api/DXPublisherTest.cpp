// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

// Segfaults appear sporadically on Linux and MacOS.
// #ifdef _WIN32

TEST_CASE("DXPublisher::publishEvents") {
    System::setProperty("dxendpoint.eventTime", "true");
    System::setProperty("dxscheme.nanoTime", "true");

    auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    auto feed = endpoint->getFeed();
    auto publisher = endpoint->getPublisher();
    auto sub = feed->createSubscription({Quote::TYPE});
    auto quote = std::make_shared<Quote>("AAPL");

    quote->setEventTime(1692975409000);
    quote->setTimeNanoPart(123);
    quote->setSequence(123);
    quote->setBidTime(1692975409000);
    quote->setBidExchangeCode('B');
    quote->setBidPrice(176.08);
    quote->setBidSize(1.0);
    quote->setAskTime(1692975409000);
    quote->setAskExchangeCode('A');
    quote->setAskPrice(176.16);
    quote->setAskSize(2.0);

    std::vector<std::shared_ptr<EventType>> result{};
    std::mutex mtx{};

    sub->addEventListener([&mtx, &result](const auto &events) {
        for (const auto &e : events) {
            if (const auto &q = e->template sharedAs<Quote>(); q) {
                std::cout << q << std::endl;

                std::lock_guard lock{mtx};
                result.push_back(std::make_shared<Quote>(*q));
            }
        }
    });

    sub->addSymbols("AAPL");
    publisher->publishEvents(quote);
    publisher->publishEvents({quote, quote});

    std::vector<EventType::Ptr> events{};
    events.push_back(quote);
    events.push_back(quote);

    publisher->publishEvents(events);

    std::this_thread::sleep_for(1000ms);

    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();

    std::lock_guard lock{mtx};

    REQUIRE_EQ(result.size(), 1);
    REQUIRE_EQ(quote->toString(), result.back()->toString());
}

// #endif

namespace {

// A kind of invalid UTF-8 after a valid prefix: the suffix and the bytes the error names.
struct InvalidUtf8 {
    const char *kind;
    const char *suffix;
    const char *bytes;
};

const std::vector<InvalidUtf8> INVALID_UTF8 = {
    {"a stray byte", "\xFF", "0xFF"},
    {"a broken sequence", "\xC3\x28", "0xC3 0x28"},
    {"a truncated sequence", "\xE2\x82", "0xE2 0x82"},
    {"an overlong form", "\xC0\xAF", "0xC0 0xAF"},
    {"an encoded surrogate", "\xED\xA0\x80", "0xED 0xA0 0x80"},
};

// Cyrillic and an emoji (U+1F600).
constexpr auto VALID_MULTIBYTE = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xF0\x9F\x98\x80";

} // namespace

TEST_CASE("DXPublisher::publishEvents: an event field with invalid UTF-8 throws with the type, the field and the bytes") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto publisher = endpoint->getPublisher();

    for (const auto &[kind, suffix, bytes] : INVALID_UTF8) {
        CAPTURE(kind);
        const auto timeAndSale = std::make_shared<TimeAndSale>("AAPL");
        const auto expected = "TimeAndSale::exchangeSaleConditions: invalid UTF-8 at byte 1: "s + bytes;

        // The setter does not check: the string is checked when the event goes to the SDK.
        CHECK_NOTHROW(timeAndSale->setExchangeSaleConditions("a"s + suffix));
        CHECK_THROWS_WITH_AS(publisher->publishEvents(timeAndSale), expected.c_str(), InvalidArgumentException);
    }

    const auto timeAndSale = std::make_shared<TimeAndSale>("AAPL");

    // Not exchangeSaleConditions: the SDK keeps it as a short string of a few characters.
    timeAndSale->setBuyer(VALID_MULTIBYTE);
    CHECK_NOTHROW(publisher->publishEvents(timeAndSale));
    CHECK_NOTHROW(publisher->publishEvents(std::make_shared<TextMessage>("AAPL", VALID_MULTIBYTE)));

    endpoint->close();
}

TEST_CASE("DXPublisher::publishEvents: an event symbol with invalid UTF-8 names the event type") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto publisher = endpoint->getPublisher();

    CHECK_THROWS_WITH_AS(publisher->publishEvents(std::make_shared<Quote>("AAPL\xC0\xAF")),
                         "Quote::eventSymbol: invalid UTF-8 at byte 4: 0xC0 0xAF", InvalidArgumentException);
    // The symbol is filled by the base classes (Order, OrderBase, MarketEvent).
    CHECK_THROWS_WITH_AS(publisher->publishEvents(std::make_shared<AnalyticOrder>("\xFF")),
                         "AnalyticOrder::eventSymbol: invalid UTF-8 at byte 0: 0xFF", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(publisher->publishEvents(std::make_shared<TextMessage>("AAPL\xFF", "text")),
                         "TextMessage::eventSymbol: invalid UTF-8 at byte 4: 0xFF", InvalidArgumentException);

    endpoint->close();
}

TEST_CASE("DXPublisher::publishEvents: an invalid event in a list frees the events converted before it") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    const auto publisher = endpoint->getPublisher();
    const auto valid = std::make_shared<TimeAndSale>("AAPL");
    const auto invalid = std::make_shared<TimeAndSale>("AAPL");

    valid->setBuyer("buyer");
    valid->setSeller("seller");
    invalid->setBuyer("buyer");
    invalid->setSeller("s\xFF");

    // Under the sanitizers a leak of the first event or of the strings of the second one fails the run.
    CHECK_THROWS_WITH_AS(publisher->publishEvents({valid, invalid}),
                         "TimeAndSale::seller: invalid UTF-8 at byte 1: 0xFF", InvalidArgumentException);
    CHECK_NOTHROW(publisher->publishEvents({valid, valid}));

    endpoint->close();
}