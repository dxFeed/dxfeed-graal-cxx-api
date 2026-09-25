// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("DXRNDP") {
    auto e1 = DXEndpoint::getInstance(DXEndpoint::Role::FEED);
    auto e2 = DXEndpoint::getInstance(DXEndpoint::Role::LOCAL_HUB);
    auto e3 = DXEndpoint::getInstance(DXEndpoint::Role::ON_DEMAND_FEED);
    auto e4 = DXEndpoint::getInstance(DXEndpoint::Role::PUBLISHER);
    auto e5 = DXEndpoint::getInstance(DXEndpoint::Role::STREAM_FEED);
    auto e6 = DXEndpoint::getInstance(DXEndpoint::Role::STREAM_PUBLISHER);

    for (auto e : {e1, e2, e3, e4, e5, e6}) {
        std::cout << e->toString() << " - " << DXEndpoint::roleToString(e->getRole()) << "\n";

        for (auto et: e->getEventTypes()) {
            std::cout << et.getName() << ", ";
        }

        std::cout << "\n\n";
    }
}

namespace {

// Records state changes reported by the listeners of an endpoint (they are called from the QD threads).
struct StateChanges {
    std::mutex mutex{};
    std::vector<std::pair<DXEndpoint::State, DXEndpoint::State>> changes{};

    void add(DXEndpoint::State oldState, DXEndpoint::State newState) {
        std::lock_guard lock{mutex};
        changes.emplace_back(oldState, newState);
    }

    bool contains(DXEndpoint::State oldState, DXEndpoint::State newState) {
        std::lock_guard lock{mutex};

        return std::find(changes.begin(), changes.end(), std::pair{oldState, newState}) != changes.end();
    }
};

} // namespace

TEST_CASE("DXEndpoint::Builder: the state follows connect, disconnect and close") {
    using State = DXEndpoint::State;

    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();
    StateChanges changes1{};
    StateChanges changes2{};

    endpoint->onStateChange() += [&changes1](State oldState, State newState) {
        changes1.add(oldState, newState);
    };

    endpoint->addStateChangeListener([&changes2](State oldState, State newState) {
        changes2.add(oldState, newState);
    });

    REQUIRE(endpoint->getState() == State::NOT_CONNECTED);

    endpoint->connect(server.getAddress());
    REQUIRE(test::waitUntil([&] {
        return endpoint->getState() == State::CONNECTED;
    }));

    endpoint->disconnect();
    REQUIRE(endpoint->getState() == State::NOT_CONNECTED);

    endpoint->connect(server.getAddress());
    REQUIRE(test::waitUntil([&] {
        return endpoint->getState() == State::CONNECTED;
    }));

    endpoint->close();
    REQUIRE(endpoint->getState() == State::CLOSED);

    for (auto *changes : {&changes1, &changes2}) {
        REQUIRE(test::waitUntil([&] {
            return changes->contains(State::CONNECTED, State::CLOSED);
        }));
        REQUIRE(changes->contains(State::NOT_CONNECTED, State::CONNECTING));
        REQUIRE(changes->contains(State::CONNECTING, State::CONNECTED));
        REQUIRE(changes->contains(State::CONNECTED, State::NOT_CONNECTED));
    }
}

namespace {

struct CStateChanges {
    std::mutex mutex{};
    std::vector<std::pair<dxfc_dxendpoint_state_t, dxfc_dxendpoint_state_t>> changes{};

    bool contains(dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState) {
        std::lock_guard lock{mutex};

        return std::find(changes.begin(), changes.end(), std::pair{oldState, newState}) != changes.end();
    }
};

dxfc_dxendpoint_state_t cApiGetState(dxfc_dxendpoint_t endpoint) {
    dxfc_dxendpoint_state_t state{};

    REQUIRE(dxfc_dxendpoint_get_state(endpoint, &state) == DXFC_EC_SUCCESS);

    return state;
}

} // namespace

TEST_CASE("dxfc_dxendpoint_builder_t: the state follows connect, disconnect and close") {
    const test::LoopbackTcpServer server{};
    const auto clientAddress = server.getAddress();
    dxfc_dxendpoint_builder_t builder{};

    REQUIRE(dxfc_dxendpoint_new_builder(&builder) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_builder_with_role(builder, DXFC_DXENDPOINT_ROLE_FEED) == DXFC_EC_SUCCESS);

    CStateChanges changes{};
    dxfc_dxendpoint_t endpoint{};

    REQUIRE(dxfc_dxendpoint_builder_build(builder, &changes, &endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_add_state_change_listener(
                endpoint,
                [](dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState, void *userData) {
                    auto *c = static_cast<CStateChanges *>(userData);
                    std::lock_guard lock{c->mutex};
                    c->changes.emplace_back(oldState, newState);
                }) == DXFC_EC_SUCCESS);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_NOT_CONNECTED);

    REQUIRE(dxfc_dxendpoint_connect(endpoint, clientAddress.c_str()) == DXFC_EC_SUCCESS);
    REQUIRE(test::waitUntil([&] {
        return cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CONNECTED;
    }));

    REQUIRE(dxfc_dxendpoint_disconnect(endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_NOT_CONNECTED);

    REQUIRE(dxfc_dxendpoint_connect(endpoint, clientAddress.c_str()) == DXFC_EC_SUCCESS);
    REQUIRE(test::waitUntil([&] {
        return cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CONNECTED;
    }));

    REQUIRE(dxfc_dxendpoint_close(endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CLOSED);
    REQUIRE(test::waitUntil([&] {
        return changes.contains(DXFC_DXENDPOINT_STATE_CONNECTED, DXFC_DXENDPOINT_STATE_CLOSED);
    }));
    REQUIRE(changes.contains(DXFC_DXENDPOINT_STATE_NOT_CONNECTED, DXFC_DXENDPOINT_STATE_CONNECTING));
    REQUIRE(changes.contains(DXFC_DXENDPOINT_STATE_CONNECTING, DXFC_DXENDPOINT_STATE_CONNECTED));
    REQUIRE(changes.contains(DXFC_DXENDPOINT_STATE_CONNECTED, DXFC_DXENDPOINT_STATE_NOT_CONNECTED));

    REQUIRE(dxfc_dxendpoint_free(endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_builder_free(builder) == DXFC_EC_SUCCESS);
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

TEST_CASE("DXEndpoint::getFeed and getPublisher") {
    auto feed = DXEndpoint::getInstance()->getFeed();
    auto publisher = DXEndpoint::getInstance()->getPublisher();

    REQUIRE(DXFeed::getInstance() == feed);
    REQUIRE(DXPublisher::getInstance() == publisher);
}

TEST_CASE("Test DXEndpoint + multi-thread setProperty") {
    std::thread t{[] {
        System::setProperty("test", "test");
    }};
    t.join();

    auto endpoint = DXEndpoint::create();

    for (int i = 0; i < 1000; ++i) {
        std::thread t2{[] {
            System::setProperty("test", "test");
        }};

        t2.join();
    }

    endpoint->close();
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
}