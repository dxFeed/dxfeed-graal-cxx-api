// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <chrono>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

TEST_CASE("System properties can be set, as well as get their values") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(1024);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "Привет");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated.") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123123123123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "1231");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated (Unicode).") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "Пр");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.") {
    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("UnknownPropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()).empty());
}

TEST_CASE("Buffer pointer is NULL and buffer size is 0 - OK") {
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 100) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 0) == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    CHECK(dxfc_system_get_property("PropertyName", buffer.data(), 0) == DXFC_EC_SUCCESS);
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread") {
    dxfc_error_code_t errorCode = DXFC_EC_ERROR;
    // TODO: move to jthread on clang
    std::thread t([&errorCode] {
        errorCode = dxfc_system_set_property("PropertyName", "123");
    });
    t.join();
    REQUIRE(errorCode == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());
    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "123") == true);
    CHECK(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "Привет") == true);
    CHECK(dxfcpp::System::getProperty("PropertyName") == "Привет");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.") {
    CHECK(dxfcpp::System::getProperty("UnknownPropertyName").empty());
}

TEST_CASE("System properties round-trip through the SDK for every kind of string argument") {
    // "Privet" in Cyrillic, written as UTF-8 bytes.
    const std::string utf8 = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

    SUBCASE("const char *") {
        const char *value = "const-char-pointer";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.ptr", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.ptr") == "const-char-pointer");
    }

    SUBCASE("std::string") {
        const std::string value = "std-string";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.str", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.str") == value);
    }

    SUBCASE("std::string_view") {
        const std::string_view value{"string-view-with-tail", 11};

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.view", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.view") == "string-view");
    }

    SUBCASE("a literal") {
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.literal", "literal"));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.literal") == "literal");
    }

    SUBCASE("a char buffer") {
        char buffer[16] = "buffer";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.buffer", buffer));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.buffer") == "buffer");
    }

    SUBCASE("non-ASCII UTF-8") {
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.utf8", utf8));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.utf8") == utf8);
    }

    SUBCASE("a null const char * value is an empty value") {
        const char *value = nullptr;

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.null", "not empty"));
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.null", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.null").empty());
    }
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread") {
    bool ok = false;
    // TODO: move to jthread on clang
    std::thread t([&ok] {
        ok = dxfcpp::System::setProperty("PropertyName", "123");
    });
    t.join();
    REQUIRE(ok == true);

    CHECK(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("A key or a value with invalid UTF-8 throws and does not change the property") {
    using dxfcpp::InvalidArgumentException;
    using dxfcpp::System;

    SUBCASE("A value") {
        REQUIRE(System::setProperty("InvalidUtf8Value", "before"));
        CHECK_THROWS_WITH_AS(System::setProperty("InvalidUtf8Value", "abc\xe2\x28\xa1"),
                             "System::setProperty(value): invalid UTF-8 at byte 3: 0xE2 0x28",
                             InvalidArgumentException);
        CHECK(System::getProperty("InvalidUtf8Value") == "before");
    }

    SUBCASE("A key") {
        CHECK_THROWS_WITH_AS(System::setProperty("InvalidUtf8Key\xff", "value"),
                             "System::setProperty(key): invalid UTF-8 at byte 14: 0xFF", InvalidArgumentException);
        CHECK(System::getProperty("InvalidUtf8Key").empty());
        CHECK_THROWS_WITH_AS(System::getProperty("\xed\xa0\x80"),
                             "System::getProperty(key): invalid UTF-8 at byte 0: 0xED 0xA0 0x80",
                             InvalidArgumentException);
    }

    SUBCASE("The password of the endpoint is named without its bytes") {
        CHECK_THROWS_WITH_AS(System::setProperty(dxfcpp::DXEndpoint::DXFEED_PASSWORD_PROPERTY, "p\xc3\x28"),
                             "System::setProperty(value): invalid UTF-8 at byte 1", InvalidArgumentException);
    }

    SUBCASE("The C API returns an error") {
        CHECK(dxfc_system_set_property("InvalidUtf8C", "\xc0\xaf") == DXFC_EC_ERROR);
    }

    SUBCASE("A valid multibyte value") {
        REQUIRE(System::setProperty("ValidUtf8", "Значение \xF0\x9F\x98\x80"));
        CHECK(System::getProperty("ValidUtf8") == "Значение \xF0\x9F\x98\x80");
    }
}

TEST_CASE("Runtime statistics: a default object has every value not given") {
    using namespace dxfcpp;

    const RuntimeStatistics statistics{};

    CHECK(statistics.heapUsedBytes == -1);
    CHECK(statistics.heapCommittedBytes == -1);
    CHECK(statistics.heapMaxBytes == -1);
    CHECK(statistics.youngGcCount == -1);
    CHECK(statistics.youngGcTimeMs == -1);
    CHECK(statistics.fullGcCount == -1);
    CHECK(statistics.fullGcTimeMs == -1);
    CHECK(statistics.threadCount == -1);
    CHECK(statistics.peakThreadCount == -1);
    CHECK(statistics.processCpuTimeNs == -1);
    CHECK(statistics.uptimeMs == -1);
    CHECK(statistics.allocatedBytes == -1);
    CHECK(statistics.edenUsedBytes == -1);
    CHECK(statistics.survivorUsedBytes == -1);
    CHECK(statistics.oldUsedBytes == -1);
    CHECK(statistics.daemonThreadCount == -1);
    CHECK(statistics.totalStartedThreadCount == -1);
    CHECK(statistics.availableProcessors == -1);

    const auto string = statistics.toString();

    CHECK(string.starts_with("RuntimeStatistics{"));
    CHECK(string.find("heapUsedBytes = -1") != std::string::npos);
    CHECK(string.find("availableProcessors = -1}") != std::string::npos);
}

TEST_CASE("Runtime statistics of the isolate are read on the Serial GC") {
    using namespace dxfcpp;

    const auto first = System::getRuntimeStatistics();

    CHECK(first.heapUsedBytes > 0);
    CHECK(first.heapCommittedBytes > 0);
    CHECK(first.heapMaxBytes > 0);
    CHECK(first.youngGcCount >= 0);
    CHECK(first.youngGcTimeMs >= 0);
    CHECK(first.fullGcCount >= 0);
    CHECK(first.fullGcTimeMs >= 0);
    CHECK(first.threadCount > 0);
    CHECK(first.peakThreadCount > 0);
    CHECK(first.processCpuTimeNs >= 0);
    CHECK(first.uptimeMs >= 0);
    CHECK((first.allocatedBytes == -1 || first.allocatedBytes >= 0));
    CHECK(first.edenUsedBytes >= 0);
    CHECK(first.survivorUsedBytes >= 0);
    CHECK(first.oldUsedBytes >= 0);
    CHECK(first.daemonThreadCount >= 0);
    CHECK(first.totalStartedThreadCount > 0);
    CHECK(first.availableProcessors > 0);
    CHECK(first.toString().find("heapMaxBytes = " + std::to_string(first.heapMaxBytes)) != std::string::npos);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const auto second = System::getRuntimeStatistics();

    CHECK(second.uptimeMs > first.uptimeMs);
    CHECK(second.heapMaxBytes > 0);
}

TEST_CASE("The build info of the SDK library gives the version of QDS, the runtime, the machine and the GCs") {
    using namespace dxfcpp;

    const auto qdVersion = System::getBuildInfo(System::BUILD_INFO_QD_VERSION);

    REQUIRE(qdVersion.has_value());
    // For example 3.358: the digits of the version start it.
    CHECK(std::regex_search(*qdVersion, std::regex{R"(^\d+\.\d+)"}));

    const auto vendor = System::getBuildInfo(System::BUILD_INFO_VENDOR);

    REQUIRE(vendor.has_value());
    CHECK_FALSE(vendor->empty());

    // The desktop libraries are built for a target machine; only the iOS ones have none.
    const auto march = System::getBuildInfo(System::BUILD_INFO_MARCH);

    REQUIRE(march.has_value());
    CHECK_FALSE(march->empty());

    const auto gc = System::getBuildInfo(System::BUILD_INFO_GC);

    REQUIRE(gc.has_value());
    CHECK_FALSE(gc->empty());

    CHECK(System::getBuildInfo(std::string{System::BUILD_INFO_QD_VERSION}) == qdVersion);
}

TEST_CASE("The build info of an unknown or an empty key is empty, of an invalid key throws") {
    using namespace dxfcpp;

    CHECK_FALSE(System::getBuildInfo("no_such_key").has_value());
    CHECK_FALSE(System::getBuildInfo("").has_value());
    CHECK_THROWS_WITH_AS(System::getBuildInfo("qd_version\xff"),
                         "System::getBuildInfo(key): invalid UTF-8 at byte 10: 0xFF", InvalidArgumentException);
}
