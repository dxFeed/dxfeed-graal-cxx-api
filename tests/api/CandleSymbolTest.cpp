// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <fmt/format.h>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "CandleAttributesJavaParity.hpp"

using namespace dxfcpp;

using stringOpt = std::optional<std::string>;
using CandlePeriodOpt = std::optional<CandlePeriod>;

TEST_CASE("Candle Periods") {
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("DAY"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("1DAY"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("1day"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("1Day"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("Day"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("Days"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("d"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::DAY} == CandlePeriod::parse("1d"));

    REQUIRE(CandlePeriodOpt{CandlePeriod::TICK} == CandlePeriod::parse("TICK"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::TICK} == CandlePeriod::parse("tick"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::TICK} == CandlePeriod::parse("Ticks"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::TICK} == CandlePeriod::parse("1tick"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::TICK} == CandlePeriod::parse("1T"));

    REQUIRE(CandlePeriodOpt{CandlePeriod::valueOf(5, CandleType::MINUTE)} == CandlePeriod::parse("5MINUTE"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::valueOf(5, CandleType::MINUTE)} == CandlePeriod::parse("5min"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::valueOf(5, CandleType::MINUTE)} == CandlePeriod::parse("5Min"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::valueOf(5, CandleType::MINUTE)} == CandlePeriod::parse("5m"));
    REQUIRE(CandlePeriodOpt{CandlePeriod::valueOf(5, CandleType::MINUTE)} == CandlePeriod::parse("5m"));

    auto set = std::unordered_set<CandlePeriod>{};

    double val = 0.0;

    for (auto i = 0; i < 20; i++) {
        for (const auto &candleTypeRef : CandleType::VALUES) {
            const auto &type = candleTypeRef.get();

            CandlePeriod p = CandlePeriod::valueOf(val, type);

            REQUIRE(set.emplace(p).second == true); //"All should be different"

            auto s1 = p.toString();

            REQUIRE(CandlePeriodOpt{p} == CandlePeriod::parse(s1));

            auto s2 = fmt::format("{}{}", std::to_string(val), type.getName());

            INFO("val = ", val, ", type.getName() = ", type.getName(), ", type.toString() = ", type.toString(),
                 ", p.toString() = ", p.toString(), ", s2 = ", s2,
                 ", CandlePeriod::parse(s2).toString() = ", CandlePeriod::parse(s2).toString());

            REQUIRE(CandlePeriodOpt{p} == CandlePeriod::parse(s2));
        }

        val += 0.25;
    }
}

TEST_CASE("Normalization") {
    REQUIRE("IBM&E" == CandleSymbol::valueOf("IBM", CandleExchange::valueOf('E')).toString());
    REQUIRE("IBM" == CandleSymbol::valueOf("IBM", CandleExchange::COMPOSITE).toString());

    REQUIRE("IBM{=d}" == CandleSymbol::valueOf("IBM", CandlePeriod::DAY).toString());
    REQUIRE("IBM" == CandleSymbol::valueOf("IBM", CandlePeriod::TICK).toString());

    REQUIRE("IBM{price=ask}" == CandleSymbol::valueOf("IBM", CandlePrice::ASK).toString());
    REQUIRE("IBM" == CandleSymbol::valueOf("IBM", CandlePrice::LAST).toString());

    REQUIRE("IBM{tho=true}" == CandleSymbol::valueOf("IBM", CandleSession::REGULAR).toString());
    REQUIRE("IBM" == CandleSymbol::valueOf("IBM", CandleSession::ANY).toString());

    REQUIRE(
        "EUR/USD{=2h,price=bid,source=bank}" ==
        CandleSymbol::valueOf("EUR/USD{source=bank}", {CandlePrice::BID, CandlePeriod::valueOf(2, CandleType::HOUR)})
            .toString());
    REQUIRE("IBM{=15m,aa=zz,price=bid}" ==
            CandleSymbol::valueOf("IBM{aa=zz,price=b}", CandlePeriod::valueOf(15, CandleType::MINUTE)).toString());
}

TEST_CASE("Double checks") {
    REQUIRE("AAPL{=15m}" == CandleSymbol::valueOf("AAPL", {CandlePeriod::valueOf(15, CandleType::MINUTE)}).toString());
    REQUIRE(
        "AAPL{=15m,price=bid}" ==
        CandleSymbol::valueOf("AAPL", {CandlePrice::BID, CandlePeriod::valueOf(15, CandleType::MINUTE)}).toString());
    REQUIRE("AAPL{=15m}" ==
            CandleSymbol::valueOf("AAPL", std::vector{CandlePeriod::valueOf(15, CandleType::MINUTE)}).toString());
    REQUIRE("AAPL{=15m}" ==
            CandleSymbol::valueOf(
                "AAPL", std::vector<CandleSymbolAttributeVariant>{CandlePeriod::valueOf(15, CandleType::MINUTE)})
                .toString());
}

TEST_CASE("CandlePeriod string representation should round-trip small values") {
    const auto period = CandlePeriod::valueOf(1.0e-8, CandleType::PRICE);

    CHECK(CandlePeriod::parse(period.toString()) == period);
    CHECK(period.getValue() == 1.0e-8);
}

TEST_CASE("CandlePriceLevel should follow Java validation and equality semantics") {
    CHECK(CandlePriceLevel::DEFAULT == CandlePriceLevel::DEFAULT);
    CHECK(CandlePriceLevel::valueOf(std::numeric_limits<double>::quiet_NaN()) == CandlePriceLevel::DEFAULT);
    CHECK(CandlePriceLevel::DEFAULT.changeAttributeForSymbol("AAPL{pl=1}") == "AAPL");
    CHECK(CandlePriceLevel::valueOf(1.0).toString() == "1");
    CHECK(CandlePriceLevel::valueOf(0.0) != CandlePriceLevel::valueOf(1.0e-20));

    CHECK_THROWS_AS(CandlePriceLevel::valueOf(-1.0), InvalidArgumentException);
    CHECK_THROWS_AS(CandlePriceLevel::valueOf(-0.0), InvalidArgumentException);
    CHECK_THROWS_AS(CandlePriceLevel::valueOf(std::numeric_limits<double>::infinity()), InvalidArgumentException);
    CHECK_THROWS_AS(CandlePriceLevel::parse("1garbage"), InvalidArgumentException);
}

TEST_CASE("CandleSession attribute lookup should use Boolean.parseBoolean semantics") {
    CHECK(CandleSession::getAttributeForSymbol("AAPL{tho=true}").get() == CandleSession::REGULAR);
    CHECK(CandleSession::getAttributeForSymbol("AAPL{tho=TRUE}").get() == CandleSession::REGULAR);
    CHECK(CandleSession::getAttributeForSymbol("AAPL{tho=t}").get() == CandleSession::DEFAULT);
    CHECK(CandleSession::getAttributeForSymbol("AAPL{tho=garbage}").get() == CandleSession::DEFAULT);
}

TEST_CASE("CandleSymbol should report invalid attributes as exceptions") {
    CHECK_THROWS_AS(CandleSymbol::valueOf("AAPL{=garbage}"), InvalidArgumentException);
}

TEST_CASE("A default-constructed CandleSession should be CandleSession::ANY") {
    const CandleSession session{};

    CHECK(session == CandleSession::ANY);
    CHECK(session == CandleSession::DEFAULT);
    CHECK(session.toString() == "false");
    CHECK(session.getSessionFilter() == SessionFilter::ANY);
}

namespace {

// Where C++ deliberately differs from Java: Java keeps the default attributes (its normalization drops the result of
// the removal), C++ removes them. The attributes themselves are the same as in Java (checked below).
const std::map<std::string, std::string> CXX_STR_OVERRIDES = {
    {"A{=t}", "A"},
    {"A{=1t}", "A"},
    {"A{tho=false}", "A"},
    {"A{tho=t}", "A"},
    {"A{tho=yes}", "A"},
    {"A{tho=}", "A"},
    {"A{price=last}", "A"},
    {"A{price=l}", "A"},
    {"A{price=la}", "A"},
    {"A{a=m}", "A"},
    {"A{a=M}", "A"},
    {"A{pl=NaN}", "A"},
    {"A{=t,pl=NaN}", "A"},
    {"A{pl=NaN,tho=false,a=m,price=last,=t}", "A"},
    {"A{tho=false,tho=true}", "A"},
    {"A{=t,=5m}", "A"},
    {"A&Q{=t}", "A&Q"},
    {"{=t}", ""},
    {"A{{=t}", "A{"},
    {"A{=t,tho=true}", "A{tho=true}"},
    {"A{=t,tho=false}", "A"},
    {"A{=1t,tho=false}", "A"},
    {"A{tho=false,=1t}", "A"},
    {"A{=1t,pl=NaN,a=m}", "A"},
    {"A{=1t,pl=NaN,a=s}", "A{a=s}"},
    {"A{pl=NaN,price=ASK}", "A{price=ask}"},
    {"A{pl=NaN,price=last,a=s}", "A{a=s}"},
    {"A{pl=NaN,price=last,a=s,tho=TRUE}", "A{a=s,tho=true}"},
    {"B{=Tick}", "B"},
    {"B{=Ticks}", "B"},
    {"A{tho=false,tho=false}", "A"},
    {"A{tho=FALSE}", "A"},
    {"A{price=last,price=bid}", "A"},
    {"A{a=m,a=s}", "A"},
    {"A{=t,=m}", "A"},
    {"A{a=m,=t,tho=false}", "A"},
    {"A{a=m,=1t}", "A"},
    {"A{=1t,a=s}", "A{a=s}"},
    {"A{=1.0t}", "A"},
};

} // namespace

TEST_CASE("CandleSymbol::valueOf should match Java attributes and normalize idempotently") {
    for (const auto &c : candle_java_parity::SYMBOL_VALUE_OF) {
        const std::string input = c.input;

        CAPTURE(input);

        if (!c.ok) {
            CHECK_THROWS_AS(CandleSymbol::valueOf(input), InvalidArgumentException);

            continue;
        }

        std::optional<CandleSymbol> symbol{};

        CHECK_NOTHROW(symbol = CandleSymbol::valueOf(input));

        if (!symbol) {
            continue;
        }

        const auto overridden = CXX_STR_OVERRIDES.find(input);
        const std::string expected = overridden == CXX_STR_OVERRIDES.end() ? c.str : overridden->second;

        CHECK(symbol->toString() == expected);
        CHECK(symbol->getBaseSymbol() == std::string(c.base));
        CHECK(symbol->getPrice()->toString() == std::string(c.price));
        CHECK(symbol->getSession()->toString() == std::string(c.session));
        CHECK(symbol->getPeriod()->toString() == std::string(c.period));
        CHECK(symbol->getAlignment()->toString() == std::string(c.alignment));
        CHECK(symbol->getPriceLevel()->toString() == std::string(c.priceLevel));
        CHECK(CandleSymbol::valueOf(symbol->toString()).toString() == symbol->toString());
    }
}

TEST_CASE("A copy of a CandleSymbol should keep the symbol and all attributes") {
    const auto original = CandleSymbol::valueOf(
        "EUR/USD&E{source=bank}", {CandlePrice::BID, CandlePeriod::valueOf(2, CandleType::HOUR), CandleSession::REGULAR,
                                   CandleAlignment::SESSION, CandlePriceLevel::valueOf(0.5)});
    const auto check = [&original](const CandleSymbol &other) {
        CHECK(other == original);
        CHECK(other.toString() == original.toString());
        CHECK(other.getBaseSymbol() == original.getBaseSymbol());
        CHECK(other.getExchange()->toString() == original.getExchange()->toString());
        CHECK(other.getPrice() == original.getPrice());
        CHECK(other.getSession() == original.getSession());
        CHECK(other.getPeriod() == original.getPeriod());
        CHECK(other.getAlignment() == original.getAlignment());
        CHECK(other.getPriceLevel() == original.getPriceLevel());

        // The copy holds the same fields as a symbol parsed from its string.
        const auto reparsed = CandleSymbol::valueOf(other.toString());

        CHECK(other.getBaseSymbol() == reparsed.getBaseSymbol());
        CHECK(other.getExchange()->toString() == reparsed.getExchange()->toString());
        CHECK(other.getPrice() == reparsed.getPrice());
        CHECK(other.getSession() == reparsed.getSession());
        CHECK(other.getPeriod() == reparsed.getPeriod());
        CHECK(other.getAlignment() == reparsed.getAlignment());
        CHECK(other.getPriceLevel() == reparsed.getPriceLevel());
    };

    CHECK(original.toString() == "EUR/USD&E{=2h,a=s,pl=0.5,price=bid,source=bank,tho=true}");

    const CandleSymbol copy{original};

    check(copy);

    auto assigned = CandleSymbol::valueOf("IBM");

    assigned = original;
    check(assigned);

    auto moveSource = original;
    const CandleSymbol moved{std::move(moveSource)};

    check(moved);

    auto moveAssigned = CandleSymbol::valueOf("IBM");

    moveAssigned = CandleSymbol{moved};
    check(moveAssigned);
}

TEST_CASE("CandleSymbol normalization should keep a default attribute that the base symbol would take for its own") {
    for (const auto &[input, expected] : std::vector<std::pair<std::string, std::string>>{
             {"A{=t}{=t}", "A{=t}{=t}"},
             {"A{=t}{=1t}", "A{=t}{=t}"},
             {"X{=t,=bpr}{=t,=bpr}", "X{=t,=bpr}{=t}"},
             {"IB{=1M,=x}}{pl=NaN}", "IB{=1M,=x}}{pl=NaN}"},
             {"A{price=bid}{=t,price=bid}", "A{price=bid}{price=bid}"},
             {"A{b=1}{price=last}", "A{b=1}{price=last}"},
             {"A{=t,}", "A{=t,}"},
             {"A{&}{=t}", "A{&}{=t}"},
         }) {
        CAPTURE(input);

        const auto symbol = CandleSymbol::valueOf(input);

        CHECK(symbol.toString() == expected);
        CHECK(CandleSymbol::valueOf(symbol.toString()).toString() == expected);
        CHECK(symbol.getBaseSymbol() == CandleSymbol::valueOf(expected).getBaseSymbol());
    }
}
