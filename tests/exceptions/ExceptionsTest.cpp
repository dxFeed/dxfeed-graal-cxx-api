// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>
#include <string>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("JavaException") {
    try {
        throw JavaException("message", "className", "stackTrace");
    } catch (const JavaException &e) {
        std::cerr << e.what() << '\n';
        std::cerr << e.getStackTrace() << '\n';
    }
}

TEST_CASE("JavaException::throwException()") {
    try {
        JavaException::throwException();
    } catch (const JavaException &e) {
        std::cerr << e.what() << '\n';
        std::cerr << e.getStackTrace() << '\n';
    }
}

TEST_CASE("RuntimeException should put the additional stacktrace first and share the stacktrace with its copies") {
    const RuntimeException e("message", "additional");
    const RuntimeException copy{e};

    CHECK(e.getStackTrace().rfind("additional", 0) == 0);
    CHECK(&copy.getStackTrace() == &e.getStackTrace());
#ifdef DXFCXX_FEATURE_STACKTRACE
    CHECK(e.getStackTrace().size() > std::string("additional\n").size());
#else
    CHECK(e.getStackTrace() == "additional");
#endif
}
