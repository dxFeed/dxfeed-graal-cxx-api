// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The Java exception of a failed SDK call is read and cleared at once: a later, unrelated call does not get it. A null
// result or a failure status without a Java exception is an error unless the SDK function returns null as a normal
// result.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../src/internal/Isolate.hpp"
#include "../../src/isolated/IsolatedCommon.hpp"

#include <chrono>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("The Java exception of a failed call keeps its type, message, stack trace and causes") {
    try {
        JavaException::throwException();
        FAIL("no exception");
    } catch (const JavaException &e) {
        CHECK(e.getJavaClassName() == "java.lang.RuntimeException");
        CHECK(e.getJavaMessage() == "recursion");
        CHECK(std::string(e.what()) == "Java exception of type 'java.lang.RuntimeException' was thrown. recursion");
        CHECK(e.getJavaStackTrace().find("JavacNative.recursion") != std::string::npos);
        CHECK(e.getJavaStackTrace().find("Caused by: java.lang.RuntimeException: cause") != std::string::npos);
    }

    CHECK_NOTHROW(JavaException::throwIfJavaThreadExceptionExists());
}

TEST_CASE("A failure status without a Java exception is a RuntimeException") {
    try {
        runIsolatedThrow([](auto threadHandle) {
            isolated::throwThreadException(static_cast<graal_isolatethread_t *>(threadHandle));
        });
        FAIL("no exception");
    } catch (const JavaException &) {
        FAIL("a JavaException without a Java exception");
    } catch (const RuntimeException &e) {
        CHECK(std::string(e.what()).find("Unexpected failure") != std::string::npos);
    }
}

TEST_CASE("fetchThreadException takes the Java exception once") {
    runIsolatedThrow([](auto threadHandle) {
        auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);
        dxfg_v4_java_object_handler *object = nullptr;

        REQUIRE(dxfg_v4_throw_exception(thread, &object) == DXFG_EXECUTE_FAIL);

        const auto first = isolated::fetchThreadException(thread);

        REQUIRE(first.has_value());
        REQUIRE(*first != nullptr);
        CHECK(std::string((*first)->class_name) == "java.lang.RuntimeException");
        CHECK((*first)->cause != nullptr);

        const auto second = isolated::fetchThreadException(thread);

        REQUIRE(second.has_value());
        CHECK(*second == nullptr);
    });
}

TEST_CASE("throwIfNullptr throws a RuntimeException for a null result without a Java exception") {
    int *const result = nullptr;

    CHECK_THROWS_WITH_AS(JavaException::throwIfNullptr(result), doctest::Contains("Unexpected null result"),
                         RuntimeException);

    try {
        JavaException::throwIfNullptr(result);
    } catch (const JavaException &) {
        FAIL("a JavaException without a Java exception");
    } catch (const RuntimeException &) {
    }
}

TEST_CASE("nullableOrThrow returns a null result without a Java exception") {
    int *const result = nullptr;

    CHECK(JavaException::nullableOrThrow(result) == nullptr);
}

TEST_CASE("A failed call through a helper leaves no Java exception in the thread") {
    const auto endpoint = DXEndpoint::create();

    CHECK_THROWS_AS(endpoint->setRequestedAggregationPeriod(-5ms), JavaException);
    CHECK_NOTHROW(JavaException::throwIfJavaThreadExceptionExists());
}

// System::setProperty() and System::getProperty() turn a failure into a result: the Java exception must not stay in
// the thread for a later call. An empty key is an IllegalArgumentException in Java.
TEST_CASE("A failed System::setProperty() leaves no Java exception in the thread") {
    CHECK_FALSE(System::setProperty("", "value"));
    CHECK_NOTHROW(JavaException::throwIfJavaThreadExceptionExists());
}

TEST_CASE("A failed System::getProperty() leaves no Java exception in the thread") {
    CHECK(System::getProperty("").empty());
    CHECK_NOTHROW(JavaException::throwIfJavaThreadExceptionExists());
}
