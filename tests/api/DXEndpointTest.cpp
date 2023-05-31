// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include <thread>
#include <unordered_map>
#include <vector>

TEST_CASE("DXEndpoint::Builder") {
    auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
    auto endpoint = builder->build();

    endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
        std::cerr << "DXEndpoint::Builder Test: {}" + std::string("State changed: ") +
                         dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                         dxfcpp::DXEndpoint::stateToString(newState)
                  << "\n";
    };

    endpoint->addStateChangeListener([](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
        std::cerr << "DXEndpoint::Builder Test: {}" + std::string("State changed 2: ") +
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

TEST_CASE("dxfc_dxendpoint_builder_t bug") {
//    dxfc_dxendpoint_builder_t builder{};
//
//    auto result = dxfc_dxendpoint_new_builder(&builder);
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    result = dxfc_dxendpoint_builder_with_role(builder, DXFC_DXENDPOINT_ROLE_FEED);
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    dxfc_dxendpoint_t endpoint{};
//
//    result = dxfc_dxendpoint_builder_build(builder, nullptr, &endpoint);
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    result = dxfc_dxendpoint_add_state_change_listener(
//        endpoint, [](dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState, void *) {
//            std::cerr << "dxfc_dxendpoint_builder_t Test: {}" + std::string("State changed: ") +
//                             cApiStateToString(oldState) + " -> " + cApiStateToString(newState) + "\n";
//        });
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    std::this_thread::sleep_for(std::chrono::seconds(5));
//
//    result = dxfc_dxendpoint_disconnect(endpoint);
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");
//
//    if (result != DXFC_EC_SUCCESS) {
//        return;
//    }
//
//    dxfc_dxendpoint_close(endpoint);
}

TEST_CASE("DXFeedSubscription") {
    auto s0 = dxfcpp::DXFeedSubscription::create(dxfcpp::EventTypeEnum::QUOTE);
    auto s1 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE});
    auto s2 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE, dxfcpp::EventTypeEnum::CANDLE});

    dxfcpp::DXFeed::getInstance()->attachSubscription(s2);

    dxfcpp::DXFeed::getInstance();
}

TEST_CASE("dxfcpp::DXFeed::getInstance()") { dxfcpp::DXFeed::getInstance(); }
