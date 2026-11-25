// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

namespace {

// The setters that store a string copy it, so they may throw.
static_assert(!noexcept(std::declval<Quote &>().setEventSymbol(std::string{})));
static_assert(!noexcept(std::declval<Order &>().setMarketMaker(std::string{})));
static_assert(!noexcept(std::declval<TimeAndSale &>().setBuyer(std::string{})));
static_assert(!noexcept(std::declval<Message &>().setAttachment(std::string{})));
static_assert(!noexcept(Quote{std::string{}}));
static_assert(!noexcept(TradeETH{std::string{}}));
static_assert(!std::is_nothrow_constructible_v<SessionFilter, SessionFilterEnum, StringParam,
                                               std::optional<SessionType>, std::optional<bool>>);

// A user event that overrides a virtual function with a string parameter.
struct UserOrder final : Order {
    std::string seen{};

    void setEventSymbol(StringParam eventSymbol) override {
        seen = std::string{eventSymbol.view()};
        Order::setEventSymbol(eventSymbol);
    }
};

} // namespace

TEST_CASE("StringParam API: a storing setter keeps every form of the string") {
    const std::string str = "AAPL";
    const std::vector<char> vec{'A', 'A', 'P', 'L'};
    const std::array<char, 4> arr{'A', 'A', 'P', 'L'};
    char buf[16] = "AAPL";
    const char *cnull = nullptr;
    Quote quote{};

    quote.setEventSymbol("AAPL");
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(std::string_view{"AAPL"});
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(str);
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(vec);
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(std::span<const char>{arr});
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(buf);
    CHECK(quote.getEventSymbol() == "AAPL");
    quote.setEventSymbol(cnull);
    CHECK(quote.getEventSymbol().empty());

    // The setter copies: the source may change afterwards.
    std::string source = "MSFT";
    quote.setEventSymbol(source);
    source = "XXXX";
    CHECK(quote.getEventSymbol() == "MSFT");
}

TEST_CASE("StringParam API: constructors of events take a string") {
    CHECK(std::make_shared<Quote>("AAPL")->getEventSymbol() == "AAPL");

    const Quote quote{"AAPL"};

    CHECK(quote.getEventSymbol() == "AAPL");
    CHECK(TradeETH{std::string_view{"IBM"}}.getEventSymbol() == "IBM");
    CHECK(Message{"A", "attachment"}.getAttachment() == "attachment");
}

TEST_CASE("StringParam API: setMarketMaker overloads") {
    Order order{};

    order.setMarketMaker("NSDQ");
    CHECK(order.getMarketMakerOpt() == std::optional<std::string>{"NSDQ"});
    order.setMarketMaker(std::nullopt);
    CHECK(order.getMarketMakerOpt() == std::nullopt);
    order.setMarketMaker(std::optional<std::string>{"ARCA"});
    CHECK(order.getMarketMakerOpt() == std::optional<std::string>{"ARCA"});
    order.setMarketMaker(std::string_view{"BATS"});
    CHECK(order.getMarketMakerOpt() == std::optional<std::string>{"BATS"});
}

TEST_CASE("StringParam API: a user override of a virtual function receives the string") {
    UserOrder order{};
    MarketEvent &event = order;

    event.setEventSymbol(std::string_view{"IBM"});
    CHECK(order.seen == "IBM");
    CHECK(order.getEventSymbol() == "IBM");
}

TEST_CASE("StringParam API: overloads keep their choice") {
    CHECK(OrderSource::valueOf(0) == OrderSource::DEFAULT);
    CHECK(OrderSource::valueOf("NTV") == OrderSource::NTV);
    CHECK(CandleSymbol::valueOf("AAPL", {CandlePeriod::DAY}).toString() == "AAPL{=d}");
    CHECK(CandleSymbol::valueOf(std::string_view{"AAPL"}).toString() == "AAPL");
}

TEST_CASE("StringParam API: a view of a part of a buffer computes on the part only") {
    const std::string text = "AAPL{=d}XYZ";
    const std::string_view symbol = std::string_view{text}.substr(0, 8);

    CHECK(MarketEventSymbols::getBaseSymbol(symbol) == "AAPL");
    CHECK(MarketEventSymbols::getAttributeStringByKey(symbol, "") == std::optional<std::string>{"d"});
    CHECK(CandleSymbol::valueOf(symbol).toString() == "AAPL{=d}");
}

TEST_CASE("StringParam API: exceptions keep the message") {
    CHECK(std::string{RuntimeException("msg").what()} == "msg");
    CHECK(std::string{InvalidArgumentException(std::string_view{"bad"}).what()} == "bad");
}

TEST_CASE("StringParam API: DXEndpoint::Builder takes every form of a property") {
    const std::string key = DXEndpoint::NAME_PROPERTY;
    const std::vector<char> value{'n', 'a', 'm', 'e'};
    auto builder = DXEndpoint::newBuilder();

    CHECK(builder->supportsProperty(key));
    CHECK(builder->supportsProperty(std::string_view{key}));
    CHECK(builder->supportsProperty(key.c_str()));

    auto endpoint = builder->withProperty(key, value)->build();

    CHECK(endpoint->getName() == "name");
    endpoint->close();
}
