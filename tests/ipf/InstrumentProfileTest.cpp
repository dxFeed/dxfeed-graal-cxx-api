// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <string>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("InstrumentProfile::setDateField() should store a date") {
    constexpr std::int32_t dayId = 19723; // 2024-01-01
    const auto profile = InstrumentProfile::create();

    SUBCASE("Custom field") {
        profile->setDateField("TEST_DATE", dayId);

        CHECK(profile->getField("TEST_DATE") == "2024-01-01");
        CHECK(profile->getDateField("TEST_DATE") == dayId);
    }

    SUBCASE("Standard date field") {
        profile->setDateField("EXPIRATION", dayId);

        CHECK(profile->getField("EXPIRATION") == "2024-01-01");
        CHECK(profile->getDateField("EXPIRATION") == dayId);
        CHECK(profile->getExpiration() == dayId);
    }
}
