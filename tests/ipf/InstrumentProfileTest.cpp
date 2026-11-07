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

TEST_CASE("InstrumentProfile::setField() and getField() round-trip a string through the SDK") {
    const auto profile = InstrumentProfile::create();

    SUBCASE("Custom field") {
        profile->setField("TEST_FIELD", "test value");

        CHECK(profile->getField("TEST_FIELD") == "test value");
    }

    SUBCASE("Standard field, std::string and non-ASCII UTF-8") {
        // "Privet" in Cyrillic, written as UTF-8 bytes.
        const std::string utf8 = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

        profile->setField("DESCRIPTION", utf8);

        CHECK(profile->getField("DESCRIPTION") == utf8);
    }
}
