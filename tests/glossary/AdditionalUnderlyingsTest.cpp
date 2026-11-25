// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("AdditionalUnderlyings + CFI") {
    auto au1 = AdditionalUnderlyings::valueOf(std::vector<std::pair<std::string, double>>());
    auto au2 = AdditionalUnderlyings::valueOf(std::map<std::string, double>());
    auto au3 = AdditionalUnderlyings::valueOf(std::map<std::string_view, double>());
    auto au4 = AdditionalUnderlyings::valueOf(std::map<const char*, double>());
    auto au5 = AdditionalUnderlyings::valueOf("FIS 53; US$ 45.46");

    std::cout << au5 << ", hash = " << au5->hashCode() << std::endl;

    std::cout << "Map:" << std::endl;
    for (const auto & [key, value] : au5->getMap()) {
        std::cout << "  " << key << " -> " << value << std::endl;
    }

    auto cfi1 = CFI::valueOf("OPEFPS");

    auto values = cfi1->decipher();

    for (const auto &value : values) {
        std::cout << value << " |-> " << value->getDescription() << std::endl;
    }
}

TEST_CASE("AdditionalUnderlyings: text, map and SPC agree") {
    const auto fromText = AdditionalUnderlyings::valueOf("FIS 53; US$ 45.46");

    CHECK_EQ(fromText->getText(), "FIS 53; US$ 45.46");
    CHECK_EQ(fromText->getMap(), std::unordered_map<std::string, double>{{"FIS", 53.0}, {"US$", 45.46}});
    CHECK_EQ(fromText->getSPC("FIS"), doctest::Approx(53.0));
    CHECK_EQ(fromText->getSPC("US$"), doctest::Approx(45.46));
    CHECK_EQ(fromText->getSPC("IBM"), 0.0);

    CHECK_EQ(AdditionalUnderlyings::getSPC("FIS 53; US$ 45.46", "US$"), doctest::Approx(45.46));
    CHECK_EQ(AdditionalUnderlyings::getSPC("FIS 53; US$ 45.46", "IBM"), 0.0);

    const auto fromMap = AdditionalUnderlyings::valueOf(std::map<std::string, double>{{"US$", 45.46}, {"FIS", 53.0}});

    CHECK_EQ(fromMap->getText(), "FIS 53; US$ 45.46");
    CHECK_EQ(fromMap->getMap(), fromText->getMap());
    CHECK(*fromMap == *fromText);
}

TEST_CASE("AdditionalUnderlyings: an empty map is EMPTY") {
    const auto empty = AdditionalUnderlyings::valueOf(std::map<std::string, double>{});

    CHECK(empty->getText().empty());
    CHECK(empty->getMap().empty());
    CHECK(*empty == *AdditionalUnderlyings::EMPTY());
}