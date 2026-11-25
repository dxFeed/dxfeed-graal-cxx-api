// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cstddef>
#include <cstdint>
#include <string>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

TEST_SUITE("CFI") {

TEST_CASE("valueOf(string) and valueOf(intCode) agree") {
    const auto cfi = CFI::valueOf("ESNTPB");

    REQUIRE(cfi != nullptr);
    CHECK_EQ(cfi->getCode(), "ESNTPB");

    const auto byIntCode = CFI::valueOf(cfi->getIntCode());

    REQUIRE(byIntCode != nullptr);
    CHECK_EQ(byIntCode->getCode(), "ESNTPB");
    CHECK_EQ(byIntCode->getIntCode(), cfi->getIntCode());
    CHECK(*byIntCode == *cfi);
}

TEST_CASE("a short code is expanded to six characters") {
    CHECK_EQ(CFI::valueOf("ES")->getCode(), "ESXXXX");
}

TEST_CASE("an invalid code throws") {
    CHECK_THROWS_AS(CFI::valueOf("!!!!!!"), JavaException);
}

TEST_CASE("category and group") {
    const auto cfi = CFI::valueOf("ESNTPB");

    CHECK_EQ(cfi->getCategory(), static_cast<std::int16_t>('E'));
    CHECK_EQ(cfi->getCategoryChar(), 'E');
    CHECK_EQ(cfi->getGroup(), static_cast<std::int16_t>('S'));
    CHECK_EQ(cfi->getGroupChar(), 'S');
}

TEST_CASE("predicates of an equity") {
    const auto cfi = CFI::valueOf("ESNTPB");

    CHECK(cfi->isEquity());
    CHECK_FALSE(cfi->isDebtInstrument());
    CHECK_FALSE(cfi->isEntitlement());
    CHECK_FALSE(cfi->isOption());
    CHECK_FALSE(cfi->isFuture());
    CHECK_FALSE(cfi->isOther());
}

TEST_CASE("predicates of an option") {
    const auto option = CFI::valueOf("OPEFPS");

    CHECK(option->isOption());
    CHECK_FALSE(option->isEquity());
    CHECK_FALSE(option->isFuture());
}

TEST_CASE("decipher") {
    const auto cfi = CFI::valueOf("ESNTPB");
    const auto values = cfi->decipher();

    REQUIRE_FALSE(values.empty());
    CHECK_EQ(values.size(), std::size_t{6});

    for (const auto &value : values) {
        REQUIRE(value != nullptr);

        const auto attribute = value->getAttribute();

        REQUIRE(attribute != nullptr);
        CHECK_FALSE(attribute->getName().empty());
        CHECK_FALSE(value->getName().empty());
    }

    // Each value explains the corresponding character of the code.
    CHECK_EQ(values[0]->getCodeChar(), 'E');
    CHECK_EQ(values[1]->getCodeChar(), 'S');
}

TEST_CASE("the attribute lists its values") {
    const auto cfi = CFI::valueOf("ESNTPB");
    const auto value = cfi->decipher().front();
    const auto attribute = value->getAttribute();
    const auto attributeValues = attribute->getValues();

    REQUIRE_FALSE(attributeValues.empty());

    bool found = false;

    for (const auto &attributeValue : attributeValues) {
        if (attributeValue->getCode() == value->getCode()) {
            found = true;
        }
    }

    CHECK(found);
}

TEST_CASE("describe") {
    CHECK_FALSE(CFI::valueOf("ESNTPB")->describe().empty());
}

TEST_CASE("EMPTY") {
    const auto &empty = CFI::EMPTY();

    REQUIRE(empty != nullptr);
    CHECK_EQ(empty->getCode(), "XXXXXX");
    CHECK(*empty == *CFI::valueOf("XXXXXX"));
}

} // TEST_SUITE
