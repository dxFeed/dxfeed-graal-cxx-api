// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dxFeedNativeCAPI/dxFeedNativeCAPI.h>
#include <dxFeedNativeCppAPI/dxFeedNativeCppAPI.hpp>
#include <thread>
#include <unordered_map>
#include <vector>

TEST_CASE("System properties can be set, as well as get their values", "[dxfc_system]") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    REQUIRE(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)", "[dxfc_system]") {
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

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated (Unicode).", "[dxfc_system]") {
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
    // TODO: move to jthread on clang
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

TEST_CASE("System properties can be set, as well as get their values (Unicode)", "[dxfcpp::System]") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "Привет") == true);
    REQUIRE(dxfcpp::System::getProperty("PropertyName") == "Привет");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.", "[dxfcpp::System]") {
    REQUIRE(dxfcpp::System::getProperty("UnknownPropertyName").empty());
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread", "[dxfcpp::System]") {
    bool ok = false;
    // TODO: move to jthread on clang
    std::thread t([&ok] { ok = dxfcpp::System::setProperty("PropertyName", "123"); });
    t.join();
    REQUIRE(ok == true);

    REQUIRE(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("System properties benchmark", "[dxfcpp::System]") {
    BENCHMARK("System::setProperty") { dxfcpp::System::setProperty("PropertyName", "Привет"); };

    std::unordered_map<std::string, std::string> um{};

    BENCHMARK("std::unordered_map[]") { um["PropertyName"] = "Привет"; };
}

TEST_CASE("DXEndpoint::Builder", "[dxfcpp::DXEndpoint]") {
    auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
    auto endpoint = builder->build();

    endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
        std::cout << "State changed: " + dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                         dxfcpp::DXEndpoint::stateToString(newState) + "\n";
    };

    endpoint->addStateChangeListener([](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
                std::cout << "State changed 2: " + dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                                 dxfcpp::DXEndpoint::stateToString(newState) + "\n";
            });

    endpoint->connect("demo.dxfeed.com:7300");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    endpoint->disconnect();
    endpoint->connect("demo.dxfeed.com:7300");

    std::this_thread::sleep_for(std::chrono::seconds(5));
    endpoint->close();
}
