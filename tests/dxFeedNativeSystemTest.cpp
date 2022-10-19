// SPDX-License-Identifier: MPL-2.0

#include <catch2/catch_all.hpp>

#include <dxFeedNativeAPI.h>
#include <cstring>

TEST_CASE("123", "213123") {
    REQUIRE(dxfc_system_set_property("123", "123") == DXFC_EC_SUCCESS);

    char buffer[5] = {};

    auto result = dxfc_system_get_property("123", buffer, 5);

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(strcmp(buffer, "123") == 0);
}