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

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("DXEndpoint::Builder") {
    auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
    auto endpoint = builder->build();

    endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
        std::cerr << "DXEndpoint::Builder Test: " + std::string("State changed: ") +
                         dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                         dxfcpp::DXEndpoint::stateToString(newState)
                  << "\n";
    };

    endpoint->addStateChangeListener([](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
        std::cerr << "DXEndpoint::Builder Test: " + std::string("State changed 2: ") +
                         dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                         dxfcpp::DXEndpoint::stateToString(newState)
                  << "\n";
    });

    endpoint->connect("demo.dxfeed.com:7300");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    endpoint->disconnect();
    endpoint->connect("demo.dxfeed.com:7300");
    endpoint->close();
}

auto cApiStateToString(dxfc_dxendpoint_state_t state) {
    switch (state) {
    case DXFC_DXENDPOINT_STATE_NOT_CONNECTED:
        return "NOT_CONNECTED";
    case DXFC_DXENDPOINT_STATE_CONNECTING:
        return "CONNECTING";
    case DXFC_DXENDPOINT_STATE_CONNECTED:
        return "CONNECTED";
    case DXFC_DXENDPOINT_STATE_CLOSED:
        return "CLOSED";
    }

    return "";
}

TEST_CASE("dxfc_dxendpoint_builder_t") {
    dxfc_dxendpoint_builder_t builder{};

    auto result = dxfc_dxendpoint_new_builder(&builder);

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    result = dxfc_dxendpoint_builder_with_role(builder, DXFC_DXENDPOINT_ROLE_FEED);

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    dxfc_dxendpoint_t endpoint{};

    result = dxfc_dxendpoint_builder_build(builder, nullptr, &endpoint);

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    result = dxfc_dxendpoint_add_state_change_listener(
        endpoint, [](dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState, void *) {
            std::cerr << "dxfc_dxendpoint_builder_t Test: " + std::string("State changed: ") +
                             cApiStateToString(oldState) + " -> " + cApiStateToString(newState) + "\n";
        });

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    result = dxfc_dxendpoint_disconnect(endpoint);

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");

    if (result != DXFC_EC_SUCCESS) {
        return;
    }

    dxfc_dxendpoint_close(endpoint);

    std::this_thread::sleep_for(std::chrono::seconds(10));
}

TEST_CASE("DXFeedSubscription") {
    auto s0 = dxfcpp::DXFeedSubscription::create(dxfcpp::EventTypeEnum::QUOTE);
    auto s1 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE});
    auto s2 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE, dxfcpp::EventTypeEnum::CANDLE});

    dxfcpp::DXFeed::getInstance()->attachSubscription(s2);

    std::set types{dxfcpp::Quote::TYPE, dxfcpp::Trade::TYPE, dxfcpp::Summary::TYPE};

    auto s3 = dxfcpp::DXFeed::getInstance()->createSubscription(types);

    auto s4 = dxfcpp::DXFeed::getInstance()->createSubscription(types.begin(), types.end());

    auto s5 = dxfcpp::DXFeed::getInstance()->createSubscription(std::move(types));

    auto types2 = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};

    auto sub6 = dxfcpp::DXFeedSubscription::create(types2.begin(), types2.end());
}

TEST_CASE("dxfcpp::DXFeed::getInstance()") {
    dxfcpp::DXFeed::getInstance();
}

TEST_CASE("dxfcpp::SymbolWrapper::SymbolListUtils::toGraalList") {
    using namespace std::literals;

    auto *list = dxfcpp::SymbolWrapper::SymbolListUtils::toGraalList({"AAPL", "IBM"s, "TSLA"sv});
    auto *list2 = dxfcpp::SymbolWrapper::SymbolListUtils::toGraalList(std::vector<std::string>{"XXX", "YYY", "ZZZ"});

    auto set = std::set<dxfcpp::SymbolWrapper>{"111", "222"sv, "333"s};

    auto *list3 = dxfcpp::SymbolWrapper::SymbolListUtils::toGraalList(set.begin(), set.end());

    auto sl = dxfcpp::SymbolWrapper::SymbolListUtils::fromGraalList(list);

    std::cout << "Symbols:\n";
    for (const auto &s : sl) {
        std::cout << s.toString() << "\n";
    }

    dxfcpp::SymbolWrapper::SymbolListUtils::freeGraalList(list);
    dxfcpp::SymbolWrapper::SymbolListUtils::freeGraalList(list2);
    dxfcpp::SymbolWrapper::SymbolListUtils::freeGraalList(list3);
}

TEST_CASE("DXEndpoint::user") {
    using namespace std::literals;

    // dxfcpp::System::setProperty("log.file", "log.txt");

    auto endpoint = dxfcpp::DXEndpoint::create(dxfcpp::DXEndpoint::Role::FEED);

    endpoint->user("\xe2\x28\xa1");
    endpoint->connect("123123123");

    std::this_thread::sleep_for(std::chrono::seconds(5));
}

TEST_CASE("DXEndpoint::getFeed and getPublisher") {
    auto feed = DXEndpoint::getInstance()->getFeed();
    auto publisher = DXEndpoint::getInstance()->getPublisher();

    REQUIRE(DXFeed::getInstance() == feed);
    REQUIRE(DXPublisher::getInstance() == publisher);
}

#include <thread>

TEST_CASE("Test DXEndpoint + multi-thread setProperty") {
    std::thread t{[] {
        System::setProperty("test", "test");
    }};
    t.join();

    auto endpoint = DXEndpoint::create();

    for (int i = 0; i < 1000; ++i) {
        std::thread t{[] {
            System::setProperty("test", "test");
        }};
        t.join();
    }

    endpoint->close();

    std::cout << "!!!!!\n" << std::endl;
}

TEST_CASE("Test DXEndpoint + multi-thread setProperty v2") {
    auto endpoint = DXEndpoint::create();

    {
        std::thread t{[] {
            System::setProperty("test", "test");
        }};
        t.join();
    }

    std::this_thread::sleep_for(1s);

    endpoint->close();

    std::cout << "!!!!!\n" << std::endl;
}