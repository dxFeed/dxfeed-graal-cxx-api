// Copyright (c) 2024 Devexperts LLC.
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
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("SymbolWrapper::toStringUnderlying should return the result of toString called on the underlying symbol") {
    auto c = CandleSymbol::valueOf("AAPL&Q{=1m}");

    REQUIRE(SymbolWrapper(c).toStringUnderlying() == c.toString());
    REQUIRE(SymbolWrapper(WildcardSymbol::ALL).toStringUnderlying() == WildcardSymbol::ALL.toString());
    REQUIRE(SymbolWrapper(c).toString() != c.toString());
    REQUIRE(SymbolWrapper(WildcardSymbol::ALL).toString() != WildcardSymbol::ALL.toString());
}

