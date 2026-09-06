// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <fmt/format.h>
#include <limits>
#include <string>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

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
