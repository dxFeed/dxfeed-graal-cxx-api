// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

// operator== of the classes that wrap a Java object follows Java equals(): true for equal objects, false otherwise.

TEST_CASE("AuthToken::operator== should compare the token values") {
    const auto token = AuthToken::createBasicToken("user", "password");

    CHECK(token == token);
    CHECK(token == AuthToken::createBasicToken("user", "password"));
    CHECK(token == AuthToken::valueOf(token.toString()));
    CHECK_FALSE(token == AuthToken::createBasicToken("user", "another"));
    CHECK_FALSE(token == AuthToken::createBearerToken("token"));
    CHECK_FALSE(token == AuthToken::NULL_TOKEN());
    CHECK(token != AuthToken::createBasicToken("user", "another"));
    CHECK_FALSE(token != AuthToken::createBasicToken("user", "password"));
}

TEST_CASE("CFI::operator== should compare the codes") {
    const auto cfi = CFI::valueOf("ESXXXX");

    CHECK(*cfi == *cfi);
    CHECK(*cfi == *CFI::valueOf("ESXXXX"));
    CHECK_FALSE(*cfi == *CFI::valueOf("OPEFPS"));
}

TEST_CASE("CFI::Attribute::operator== and CFI::Value::operator== should compare the Java objects") {
    const auto values = CFI::valueOf("OPEFPS")->decipher();
    const auto sameValues = CFI::valueOf("OPEFPS")->decipher();

    REQUIRE(values.size() >= 2);
    REQUIRE(values.size() == sameValues.size());

    CHECK(*values[0] == *values[0]);
    CHECK(*values[0] == *sameValues[0]);
    CHECK_FALSE(*values[0] == *values[1]);

    const auto attribute = values[0]->getAttribute();

    CHECK(*attribute == *attribute);
    CHECK(*attribute == *sameValues[0]->getAttribute());
    CHECK_FALSE(*attribute == *values[1]->getAttribute());
}

TEST_CASE("PriceIncrements::operator== should compare the increments") {
    const auto increments = PriceIncrements::valueOf("0.01 3; 0.05");

    CHECK(*increments == *increments);
    CHECK(*increments == *PriceIncrements::valueOf("0.01 3; 0.05"));
    CHECK_FALSE(*increments == *PriceIncrements::valueOf(0.01));
}

TEST_CASE("AdditionalUnderlyings::operator== should compare the underlyings") {
    const auto underlyings = AdditionalUnderlyings::valueOf("FIS 53; US$ 45.46");

    CHECK(*underlyings == *underlyings);
    CHECK(*underlyings == *AdditionalUnderlyings::valueOf("FIS 53; US$ 45.46"));
    CHECK_FALSE(*underlyings == *AdditionalUnderlyings::valueOf("FIS 53"));
}

TEST_CASE("InstrumentProfile::operator== should compare the fields") {
    const auto profile = InstrumentProfile::create();

    profile->setSymbol("AAPL");
    profile->setType("STOCK");

    const auto copy = InstrumentProfile::create(profile);

    CHECK(*profile == *profile);
    CHECK(*profile == *copy);

    copy->setSymbol("IBM");

    CHECK_FALSE(*profile == *copy);
    CHECK(*profile != *copy);
}

TEST_CASE("TxModelListener::operator== should compare the listeners") {
    const auto listener = IndexedTxModelListener<Order>::create([](const auto &, const auto &, bool) {
    });
    const auto another = IndexedTxModelListener<Order>::create([](const auto &, const auto &, bool) {
    });

    CHECK(*listener == *listener);
    CHECK_FALSE(*listener == *another);
    CHECK(std::hash<IndexedTxModelListener<Order>>{}(*listener) ==
          std::hash<IndexedTxModelListener<Order>>{}(*listener));
}
