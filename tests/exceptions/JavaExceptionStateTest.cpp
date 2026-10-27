// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The Java exception of a failed SDK call is read and cleared at once: a later, unrelated call does not get it. A null
// result without a Java exception is an error unless the SDK function returns null as a normal result.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>

using namespace std::literals;
using namespace dxfcpp;

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
