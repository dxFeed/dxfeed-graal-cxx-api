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

using stringOpt = std::optional<std::string>;

void checkNonAttributedSymbol(const std::string &s) {
    REQUIRE(s == MarketEventSymbols::getBaseSymbol(s));
    REQUIRE("MSFT" == MarketEventSymbols::changeBaseSymbol(s, "MSFT"));
    REQUIRE(false == MarketEventSymbols::hasExchangeCode(s));
    REQUIRE('\0' == MarketEventSymbols::getExchangeCode(s));
    REQUIRE(s == MarketEventSymbols::changeExchangeCode(s, '\0'));
    REQUIRE(s + "&C" == MarketEventSymbols::changeExchangeCode(s, 'C'));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "key"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "key"));
    REQUIRE(s + "{key=val}" == MarketEventSymbols::changeAttributeStringByKey(s, "key", "val"));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, ""));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, ""));
    REQUIRE(s + "{=val}" == MarketEventSymbols::changeAttributeStringByKey(s, "", "val"));
    REQUIRE(s + "{=}" == MarketEventSymbols::changeAttributeStringByKey(s, "", ""));
}

TEST_CASE("Test an empty symbol") {
    checkNonAttributedSymbol("");
}

TEST_CASE("Test a regular symbol") {
    checkNonAttributedSymbol("IBM");
}

TEST_CASE("Test a broken symbol") {
    checkNonAttributedSymbol("{");
    checkNonAttributedSymbol("}");
    checkNonAttributedSymbol("{}");
    checkNonAttributedSymbol("A{");
    checkNonAttributedSymbol("A}");
    checkNonAttributedSymbol("A{}");
    checkNonAttributedSymbol("{B");
    checkNonAttributedSymbol("}B");
    checkNonAttributedSymbol("{}B");
}

TEST_CASE("Test regional") {
    auto s = "GE&N";

    REQUIRE("GE" == MarketEventSymbols::getBaseSymbol(s));
    REQUIRE("F&N" == MarketEventSymbols::changeBaseSymbol(s, "F"));
    REQUIRE(true == MarketEventSymbols::hasExchangeCode(s));
    REQUIRE('N' == MarketEventSymbols::getExchangeCode(s));
    REQUIRE("GE" == MarketEventSymbols::changeExchangeCode(s, '\0'));
    REQUIRE("GE&Q" == MarketEventSymbols::changeExchangeCode(s, 'Q'));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "tho"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "tho"));
    REQUIRE("GE&N{tho=true}" == MarketEventSymbols::changeAttributeStringByKey(s, "tho", "true"));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, ""));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, ""));
    REQUIRE("GE&N{=yes}" == MarketEventSymbols::changeAttributeStringByKey(s, "", "yes"));
    REQUIRE("GE&N{=}" == MarketEventSymbols::changeAttributeStringByKey(s, "", ""));
}

TEST_CASE("Test one attribute") {
    auto s = "/ES{tho=true}";

    REQUIRE("/ES" == MarketEventSymbols::getBaseSymbol(s));
    REQUIRE("/NQ{tho=true}" == MarketEventSymbols::changeBaseSymbol(s, "/NQ"));
    REQUIRE(false == MarketEventSymbols::hasExchangeCode(s));
    REQUIRE('\0' == MarketEventSymbols::getExchangeCode(s));
    REQUIRE(s == MarketEventSymbols::changeExchangeCode(s, '\0'));
    REQUIRE("/ES&G{tho=true}" == MarketEventSymbols::changeExchangeCode(s, 'G'));

    REQUIRE(stringOpt{"true"} == MarketEventSymbols::getAttributeStringByKey(s, "tho"));
    REQUIRE("/ES" == MarketEventSymbols::removeAttributeStringByKey(s, "tho"));
    REQUIRE("/ES{tho=false}" == MarketEventSymbols::changeAttributeStringByKey(s, "tho", "false"));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "t"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "t"));
    REQUIRE("/ES{t=MIN,tho=true}" == MarketEventSymbols::changeAttributeStringByKey(s, "t", "MIN"));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "zap"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "zap"));
    REQUIRE("/ES{tho=true,zap=15}" == MarketEventSymbols::changeAttributeStringByKey(s, "zap", "15"));
    REQUIRE("/ES{tho=true,zap=}" == MarketEventSymbols::changeAttributeStringByKey(s, "zap", ""));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, ""));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, ""));
    REQUIRE("/ES{=code,tho=true}" == MarketEventSymbols::changeAttributeStringByKey(s, "", "code"));
    REQUIRE("/ES{=,tho=true}" == MarketEventSymbols::changeAttributeStringByKey(s, "", ""));
}

TEST_CASE("Test two attributes") {
    auto s = "A{c=1,e=3}";

    REQUIRE("A" == MarketEventSymbols::getBaseSymbol(s));
    REQUIRE("B{c=1,e=3}" == MarketEventSymbols::changeBaseSymbol(s, "B"));
    REQUIRE(false == MarketEventSymbols::hasExchangeCode(s));
    REQUIRE('\0' == MarketEventSymbols::getExchangeCode(s));
    REQUIRE(s == MarketEventSymbols::changeExchangeCode(s, '\0'));
    REQUIRE("A&D{c=1,e=3}" == MarketEventSymbols::changeExchangeCode(s, 'D'));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "b"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "b"));
    REQUIRE("A{b=2,c=1,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "b", "2"));
    REQUIRE("A{b=,c=1,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "b", ""));

    REQUIRE(stringOpt{"1"} == MarketEventSymbols::getAttributeStringByKey(s, "c"));
    REQUIRE("A{e=3}" == MarketEventSymbols::removeAttributeStringByKey(s, "c"));
    REQUIRE("A{c=2,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "c", "2"));
    REQUIRE("A{c=,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "c", ""));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "d"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "d"));
    REQUIRE("A{c=1,d=4,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "d", "4"));
    REQUIRE("A{c=1,d=,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "d", ""));

    REQUIRE(stringOpt{"3"} == MarketEventSymbols::getAttributeStringByKey(s, "e"));
    REQUIRE("A{c=1}" == MarketEventSymbols::removeAttributeStringByKey(s, "e"));
    REQUIRE("A{c=1,e=0}" == MarketEventSymbols::changeAttributeStringByKey(s, "e", "0"));
    REQUIRE("A{c=1,e=}" == MarketEventSymbols::changeAttributeStringByKey(s, "e", ""));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, "t"));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, "t"));
    REQUIRE("A{c=1,e=3,t=5}" == MarketEventSymbols::changeAttributeStringByKey(s, "t", "5"));
    REQUIRE("A{c=1,e=3,t=}" == MarketEventSymbols::changeAttributeStringByKey(s, "t", ""));

    REQUIRE(std::nullopt == MarketEventSymbols::getAttributeStringByKey(s, ""));
    REQUIRE(s == MarketEventSymbols::removeAttributeStringByKey(s, ""));
    REQUIRE("A{=-1,c=1,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "", "-1"));
    REQUIRE("A{=,c=1,e=3}" == MarketEventSymbols::changeAttributeStringByKey(s, "", ""));
}

TEST_CASE("normalizeAttributeForSymbol") {
    auto s = "AAPL&Q{a=m}";

    REQUIRE("AAPL&Q" == CandleAlignment::normalizeAttributeForSymbol(s));
}