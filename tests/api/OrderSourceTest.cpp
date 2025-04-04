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
using namespace std::literals;

TEST_CASE("OrderSource::valueOf the method should work correctly with predefined sources") {
    REQUIRE(OrderSource::valueOf("COMPOSITE_ASK") == OrderSource::COMPOSITE_ASK);
    REQUIRE(OrderSource::valueOf("NTV") == OrderSource::NTV);
    REQUIRE(OrderSource::valueOf("NTV") != OrderSource::ntv);
    REQUIRE(OrderSource::valueOf("NTV2") != OrderSource::NTV);
}
