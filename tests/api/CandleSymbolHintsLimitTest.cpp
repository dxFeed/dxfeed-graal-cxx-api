// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The limit of the hints of CandleSymbol::valueOf(): a separate test executable, because the hints are counted for the
// whole process.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <string>
#include <vector>

using namespace dxfcpp;
using dxfcpp::test::ErrorManagerGuard;

TEST_CASE("The hints stop after 1000 candle symbols with one more warning") {
    ErrorManagerGuard guard{};
    std::vector<std::string> messages{};

    ErrorManager::setHandler([&messages](const ErrorInfo &error) {
        if (error.component == "CandleSymbol" && error.level == ErrorInfo::Level::WARN) {
            messages.push_back(error.message);
        }
    });

    for (int i = 0; i < 1000; i++) {
        CandleSymbol::valueOf("LIMIT" + std::to_string(i));
    }

    REQUIRE(messages.size() == 1001);
    CHECK(messages[999].find("'LIMIT999' has no period") != std::string::npos);
    CHECK(messages[1000].find("1000 candle symbols") != std::string::npos);

    // The next symbols get no hints, and neither do the symbols that got one.
    CandleSymbol::valueOf("LIMIT1000");
    CandleSymbol::valueOf("LIMIT1001{=5M}");
    CandleSymbol::valueOf("LIMIT0");

    CHECK(messages.size() == 1001);
}
