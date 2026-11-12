// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include "../../src/internal/Isolate.hpp"

#include <cstdint>
#include <optional>
#include <string>

using namespace dxfcpp;

TEST_CASE("isGraalNativeSdkVersionSupported() compares major.minor.patch") {
    CHECK(isGraalNativeSdkVersionSupported("3.15.0", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.15.1", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.16.0", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("4.0.0", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.100.0", "3.15.0"));
    CHECK_FALSE(isGraalNativeSdkVersionSupported("3.14.9", "3.15.0"));
    CHECK_FALSE(isGraalNativeSdkVersionSupported("3.9.0", "3.15.0"));
    CHECK_FALSE(isGraalNativeSdkVersionSupported("2.99.99", "3.15.0"));
}

TEST_CASE("isGraalNativeSdkVersionSupported() ignores the suffix") {
    CHECK(isGraalNativeSdkVersionSupported("3.15.0-SNAPSHOT", "3.15.0"));
    CHECK_FALSE(isGraalNativeSdkVersionSupported("3.14.0-SNAPSHOT", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.15.0.1", "3.15.0"));
}

TEST_CASE("isGraalNativeSdkVersionSupported() passes a version that cannot be parsed") {
    CHECK(isGraalNativeSdkVersionSupported("", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("garbage", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.14", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("v3.14.0", "3.15.0"));
    CHECK(isGraalNativeSdkVersionSupported("3.14.9999999999", "3.15.0"));
}

TEST_CASE("checkGraalNativeSdkAdvancedRevision() passes the same revision only") {
    CHECK_NOTHROW(checkGraalNativeSdkAdvancedRevision(1, 1));

    try {
        checkGraalNativeSdkAdvancedRevision(2, 1);
        FAIL("No exception");
    } catch (const GraalException &) {
        FAIL("Not a failure of the creation of the isolate");
    } catch (const RuntimeException &e) {
        const std::string message = e.what();

        CHECK(message.find("revision 2") != std::string::npos);
        CHECK(message.find("revision 1") != std::string::npos);
    }

    try {
        checkGraalNativeSdkAdvancedRevision(std::nullopt, 1);
        FAIL("No exception");
    } catch (const RuntimeException &e) {
        const std::string message = e.what();

        CHECK(message.find("revision unknown") != std::string::npos);
        CHECK(message.find("revision 1") != std::string::npos);
    }
}

TEST_CASE("The SDK library has the version and the advanced revision the library is built for") {
    // Creates the isolate: the version check passes, and the revision is read.
    CHECK_NOTHROW(Isolate::checkAdvancedRevision());
    CHECK(Isolate::getCreationError() == CEntryPointErrorsEnum::NO_ERROR);
}
