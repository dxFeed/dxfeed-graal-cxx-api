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

TEST_CASE("StringLike should preserve owned data when copied and moved") {
    auto original = StringLike(std::string{"short value"});
    auto copy = original;
    auto moved = std::move(copy);

    original = StringLike(std::string{"a different and longer value"});

    CHECK(std::string{moved} == "short value");
    CHECK(std::string{moved.c_str()} == "short value");
}

TEST_CASE("StringLike::c_str should terminate a sliced string view at its boundary") {
    const auto storage = std::string{"visible-hidden"};
    const auto slice = StringLike(std::string_view{storage}.substr(0, 7));

    CHECK(std::string{slice.c_str()} == "visible");
}
