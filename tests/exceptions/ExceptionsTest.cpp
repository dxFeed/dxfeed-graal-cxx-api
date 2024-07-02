// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("JavaException") {
    try {
        throw JavaException("message", "className", "stackTrace");
    } catch (const JavaException& e) {
        std::cerr << e.what() << '\n';
        std::cerr << e.getStackTrace() << '\n';
    }
}

TEST_CASE("JavaException::throwException()") {
    try {
        JavaException::throwException();
    } catch (const JavaException& e) {
        std::cerr << e.what() << '\n';
        std::cerr << e.getStackTrace() << '\n';
    }
}