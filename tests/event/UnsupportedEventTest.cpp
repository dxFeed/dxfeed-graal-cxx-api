// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Events of the types that the C++ API does not implement (CONFIGURATION) arrive in the event callback of a
// subscription. They must not terminate the process: they are skipped and reported to ErrorManager, the other events
// of the batch are delivered.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TapeSupport.hpp"
#include "../support/TestSupport.hpp"

#include <dxfg_api.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace dxfcpp;
using dxfcpp::test::ErrorManagerGuard;

namespace {

dxfg_quote_t createQuote(const char *symbol) {
    dxfg_quote_t quote{};

    quote.market_event.event_type.clazz = DXFG_EVENT_QUOTE;
    quote.market_event.event_symbol = symbol;

    return quote;
}

} // namespace

TEST_CASE("A subscription to CONFIGURATION delivers the other events of the batch") {
    ErrorManagerGuard guard{};
    const auto endpoint = DXEndpoint::newBuilder()
                              ->withRole(DXEndpoint::Role::STREAM_FEED)
                              ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                              ->build();
    std::mutex mutex{};
    std::vector<std::string> symbols{};
    const auto subscription = endpoint->getFeed()->createSubscription({EventTypeEnum::CONFIGURATION, Quote::TYPE});

    subscription->addEventListener([&](const std::vector<std::shared_ptr<EventType>> &events) {
        std::lock_guard lock{mutex};

        for (const auto &event : events) {
            if (const auto quote = event->sharedAs<Quote>()) {
                symbols.push_back(quote->getEventSymbol());
            }
        }
    });
    subscription->addSymbols(WildcardSymbol::ALL);

    endpoint->connect("file:" + test::testDataPath("tapes/configuration.txt") + "[speed=max]");
    endpoint->awaitNotConnected();
    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();

    std::lock_guard lock{mutex};

    REQUIRE(symbols == std::vector<std::string>{"QT"});

    // The three CONFIGURATION events may come in one batch or in several: one error per batch with its count.
    std::size_t skipped = 0;

    for (const auto &error : ErrorManager::getErrors()) {
        CHECK(error.component == "EventMapper");
        CHECK(error.message.find("of type CONFIGURATION skipped") != std::string::npos);
        skipped += static_cast<std::size_t>(std::stoul(error.message));
    }

    CHECK(skipped == 3);
}

TEST_CASE("The conversion of the events of a callback skips an event that cannot be converted") {
    ErrorManagerGuard guard{};
    auto first = createQuote("A");
    dxfg_event_type_t unknown{static_cast<dxfg_event_clazz_t>(9999)};
    auto second = createQuote("B");
    dxfg_event_type_t *elements[] = {&first.market_event.event_type, &unknown, &second.market_event.event_type};
    dxfg_event_type_list list{3, elements};

    const auto events = EventMapper::fromGraalListSkippingErrors(&list);

    REQUIRE(events.size() == 2);
    CHECK(events[0]->sharedAs<Quote>()->getEventSymbol() == "A");
    CHECK(events[1]->sharedAs<Quote>()->getEventSymbol() == "B");

    const auto errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].component == "EventMapper");
    CHECK(errors[0].message == "1 event(s) of type 9999 skipped: Unknown event type: 9999");
}

TEST_CASE("The strict conversion of a list still throws on an event type that is not implemented") {
    ErrorManagerGuard guard{};
    dxfg_event_type_t configuration{DXFG_EVENT_CONFIGURATION};
    dxfg_event_type_t *elements[] = {&configuration};
    dxfg_event_type_list list{1, elements};

    CHECK_THROWS_AS(EventMapper::fromGraalList(&list), InvalidArgumentException);
    CHECK(ErrorManager::getErrors().empty());
}

TEST_CASE("A null list of events converts to an empty vector") {
    ErrorManagerGuard guard{};

    CHECK(EventMapper::fromGraalList(nullptr).empty());
    CHECK(EventMapper::fromGraalListSkippingErrors(nullptr).empty());
    CHECK(ErrorManager::getErrors().empty());
}
