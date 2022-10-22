// SPDX-License-Identifier: MPL-2.0

#include <catch2/catch_test_macros.hpp>

#include <dxFeedNativeAPI.h>
#include <cstring>
#include <vector>

TEST_CASE("123", "213123") {
    REQUIRE(dxfc_system_set_property("123", "123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("123", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(strcmp(buffer.data(), "123") == 0);
}