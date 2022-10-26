// SPDX-License-Identifier: MPL-2.0

#include <catch2/catch_test_macros.hpp>

#include <dxFeedNativeAPI.h>
#include <dxFeedNativeCppAPI.hpp>
#include <thread>
#include <vector>

TEST_CASE("System properties can be set, as well as get their values", "[dxfc_system]") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)", "[dxfc_system][!mayfail]") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(1024);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "Привет");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated.", "[dxfc_system]") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123123123123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "1231");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated (Unicode).",
          "[dxfc_system][!mayfail]") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "Пр");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.", "[dxfc_system]") {
    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("UnknownPropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()).empty());
}

TEST_CASE("Buffer pointer is NULL and buffer size is 0 - OK", "[dxfc_system]") {
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 100) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 0) == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    REQUIRE(dxfc_system_get_property("PropertyName", buffer.data(), 0) == DXFC_EC_SUCCESS);
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread", "[dxfc_system]") {
    dxfc_error_code_t errorCode = DXFC_EC_ERROR;
    // TODO: move to jthread on macOS
    std::thread t([&errorCode] { errorCode = dxfc_system_set_property("PropertyName", "123"); });
    t.join();
    REQUIRE(errorCode == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());
    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values", "[dxfcpp::System]") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "123") == true);
    REQUIRE(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)", "[dxfcpp::System][!mayfail]") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "Привет") == true);
    REQUIRE(dxfcpp::System::getProperty("PropertyName") == "Привет");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.",
          "[dxfcpp::System][dxfc_system]") {
    REQUIRE(dxfcpp::System::getProperty("UnknownPropertyName").empty());
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread", "[dxfcpp::System]") {
    bool ok = false;
    // TODO: move to jthread on macOS
    std::thread t([&ok] { ok = dxfcpp::System::setProperty("PropertyName", "123"); });
    t.join();
    REQUIRE(ok == true);

    REQUIRE(dxfcpp::System::getProperty("PropertyName") == "123");
}