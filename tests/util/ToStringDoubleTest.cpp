// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// MDAPI-462: util/StringUtils.hpp is a public header that compiles on its own. Nothing else of the library is included
// here, so the declaration of toString(double) must not depend on internal headers.

#include <dxfeed_graal_cpp_api/util/StringUtils.hpp>

#include <doctest.h>

#include <limits>
#include <string>

TEST_CASE("toString(double) is Double.toString of Java") {
    CHECK(dxfcpp::toString(1.0) == "1.0");
    CHECK(dxfcpp::toString(1e-4) == "1.0E-4");
    CHECK(dxfcpp::toString(std::numeric_limits<double>::quiet_NaN()) == "NaN");
    CHECK(dxfcpp::toString(-0.0) == "-0.0");
    CHECK(dxfcpp::toString(std::numeric_limits<double>::infinity()) == "Infinity");
    CHECK(dxfcpp::toString(-std::numeric_limits<double>::infinity()) == "-Infinity");
    CHECK(dxfcpp::toString(1234567.0) == "1234567.0");
    CHECK(dxfcpp::toString(1.0e7) == "1.0E7");
}
