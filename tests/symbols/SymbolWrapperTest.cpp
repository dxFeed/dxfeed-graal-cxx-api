// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("SymbolWrapper::toStringUnderlying should return the result of toString called on the underlying symbol") {
    auto c = CandleSymbol::valueOf("AAPL&Q{=1m}");

    REQUIRE(SymbolWrapper(c).toStringUnderlying() == c.toString());
    REQUIRE(SymbolWrapper(WildcardSymbol::ALL).toStringUnderlying() == WildcardSymbol::ALL.toString());
    REQUIRE(SymbolWrapper(c).toString() != c.toString());
    REQUIRE(SymbolWrapper(WildcardSymbol::ALL).toString() != WildcardSymbol::ALL.toString());
}

TEST_CASE("DXFeedSubscription::addSymbols: a symbol with invalid UTF-8 throws with the symbol kind and the bytes") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto subscription = endpoint->getFeed()->createSubscription({Quote::TYPE, Order::TYPE});

    CHECK_THROWS_WITH_AS(subscription->addSymbols("\xE2\x82"s),
                         "StringSymbol(symbol): invalid UTF-8 at byte 0: 0xE2 0x82", InvalidArgumentException);
    // The symbols converted before the invalid one are freed (a leak fails the sanitizer runs).
    CHECK_THROWS_WITH_AS(subscription->addSymbols({"AAPL"s, "IBM"s, "x\xFF"s}),
                         "StringSymbol(symbol): invalid UTF-8 at byte 1: 0xFF", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(subscription->addSymbols(CandleSymbol::valueOf("AAPL\xFF")),
                         "CandleSymbol(symbol): invalid UTF-8 at byte 4: 0xFF", InvalidArgumentException);
    // The symbol is converted before the source and is freed when the source throws.
    CHECK_THROWS_WITH_AS(
        subscription->addSymbols(IndexedEventSubscriptionSymbol("AAPL", IndexedEventSource(1000, "s\xC3\x28"s))),
        "IndexedEventSource(name): invalid UTF-8 at byte 1: 0xC3 0x28", InvalidArgumentException);
    CHECK(subscription->getSymbols().empty());

    CHECK_NOTHROW(subscription->addSymbols("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"s)); // Привет
    CHECK(subscription->getSymbols().size() == 1);

    endpoint->close();
}
