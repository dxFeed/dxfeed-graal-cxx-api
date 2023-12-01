// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dxfeed_graal_cpp_api/api.hpp"
#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

TEST_CASE("OrderSource::valueOf the method should work correctly with predefined sources") {
    REQUIRE(OrderSource::valueOf("COMPOSITE_ASK") == OrderSource::COMPOSITE_ASK);
    REQUIRE(OrderSource::valueOf("NTV") == OrderSource::NTV);
    REQUIRE(OrderSource::valueOf("NTV") != OrderSource::ntv);
    REQUIRE(OrderSource::valueOf("NTV2") != OrderSource::NTV);
}
