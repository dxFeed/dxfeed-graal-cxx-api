// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("AdditionalUnderlyings + CFI") {
    auto au1 = AdditionalUnderlyings::valueOf(std::vector<std::pair<std::string, double>>());
    auto au2 = AdditionalUnderlyings::valueOf(std::map<std::string, double>());
    auto au3 = AdditionalUnderlyings::valueOf(std::map<StringLikeWrapper, double>());
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