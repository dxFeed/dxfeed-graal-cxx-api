// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// An exception of a callback that the dxFeed Graal SDK calls must not reach the SDK: it goes to ErrorManager.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp>

#include <stdexcept>
#include <string>

using namespace dxfcpp;

namespace {

struct ErrorManagerGuard {
    ErrorManagerGuard() {
        ErrorManager::setHandler({});
        ErrorManager::clearErrors();
    }

    ~ErrorManagerGuard() {
        ErrorManager::resetHandler();
        ErrorManager::clearErrors();
    }
};

} // namespace

TEST_CASE("runCallback reports an exception of the body") {
    ErrorManagerGuard guard{};

    SUBCASE("std::exception") {
        const auto line = __LINE__ + 1;
        runCallback("Test", [] { throw std::runtime_error("x"); });

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->component == "Test");
        CHECK(error->message == "x");
        CHECK(error->location == "CallbackBarrierTest.cpp:" + std::to_string(line));
    }

    SUBCASE("JavaException") {
        runCallback("Test", [] { throw JavaException("message", "java.lang.RuntimeException", "trace"); });

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->javaClassName == "java.lang.RuntimeException");
        CHECK(error->message == "message");
    }

    SUBCASE("Another exception") {
        runCallback("Test", [] { throw 42; });

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->message == "unknown exception");
    }

    SUBCASE("No exception") {
        bool called = false;

        runCallback("Test", [&] { called = true; });

        CHECK(called);
        CHECK(ErrorManager::getErrors().empty());
    }
}
