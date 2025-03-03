// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
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

    std::unordered_set set = std::unordered_set<CandlePeriod>{};

    for (double val = 0; val <= 5; val += 0.25) {
        for (const auto &candleTypeRef : CandleType::VALUES) {
            const auto &type = candleTypeRef.get();

            CandlePeriod p = CandlePeriod::valueOf(val, type);

            REQUIRE(set.emplace(p).second == true); //"All should be different"

            auto s1 = p.toString();

            REQUIRE(CandlePeriodOpt{p} == CandlePeriod::parse(s1));

            auto s2 = std::to_string(val) + type.getName();

            INFO("val = ", val, ", type.getName() = ", type.getName(), ", type.toString() = ", type.toString(),
                 ", p.toString() = ", p.toString(), ", s2 = ", s2,
                 ", CandlePeriod::parse(s2).toString() = ", CandlePeriod::parse(s2).toString());

            REQUIRE(CandlePeriodOpt{p} == CandlePeriod::parse(s2));
        }
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