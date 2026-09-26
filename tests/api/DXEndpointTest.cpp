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

// Records the state changes reported by the listeners of an endpoint. In QD (DXEndpointImpl.StateHolder) one task on
// the executor of the endpoint fires the change from the last reported state to the state computed when the task runs,
// so quick changes are reported as one (NOT_CONNECTED -> CONNECTED without CONNECTING), and a notification may arrive
// after close() returns. So the tests check how many times each state that the endpoint really reached was reported.
template <typename State> struct StateChanges {
    std::mutex mutex{};
    std::vector<std::pair<State, State>> changes{};

    void add(State oldState, State newState) {
        std::lock_guard lock{mutex};
        changes.emplace_back(oldState, newState);
    }

    std::size_t countChangesTo(State newState) {
        std::lock_guard lock{mutex};

        return static_cast<std::size_t>(std::count_if(changes.begin(), changes.end(), [newState](const auto &change) {
            return change.second == newState;
        }));
    }

    std::string toString() {
        std::lock_guard lock{mutex};
        std::string result{};

        for (const auto &[oldState, newState] : changes) {
            result += std::to_string(static_cast<int>(oldState)) + "->" + std::to_string(static_cast<int>(newState)) + " ";
        }

        return result;
    }
};

} // namespace

TEST_CASE("DXEndpoint::Builder: the state follows connect, disconnect and close") {
    using State = DXEndpoint::State;

    const test::LoopbackTcpServer server{};
    const auto executor = InPlaceExecutor::create();
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();
    StateChanges<State> changes1{};
    StateChanges<State> changes2{};

    // The notifications are delivered only by processAllPendingTasks(): the test decides when they arrive.
    endpoint->executor(executor);

    const auto listenerId1 = endpoint->onStateChange() += [&changes1](State oldState, State newState) {
        changes1.add(oldState, newState);
    };

    const auto listenerId2 = endpoint->addStateChangeListener([&changes2](State oldState, State newState) {
        changes2.add(oldState, newState);
    });

    // The endpoint also runs its own tasks of the connection process through the executor.
    const auto waitForState = [&](State state) {
        return test::waitUntil([&] {
            executor->processAllPendingTasks();

            return endpoint->getState() == state;
        });
    };

    const auto waitForChangeTo = [&](State newState, std::size_t times) {
        const auto result = test::waitUntil([&] {
            executor->processAllPendingTasks();

            return changes1.countChangesTo(newState) == times && changes2.countChangesTo(newState) == times;
        });

        INFO("state changes: ", changes1.toString(), "/ ", changes2.toString());
        CHECK(result);

        return result;
    };

    REQUIRE(endpoint->getState() == State::NOT_CONNECTED);

    endpoint->connect(server.getAddress());
    REQUIRE(waitForState(State::CONNECTED));
    REQUIRE(waitForChangeTo(State::CONNECTED, 1));

    endpoint->disconnect();
    REQUIRE(endpoint->getState() == State::NOT_CONNECTED);
    REQUIRE(waitForChangeTo(State::NOT_CONNECTED, 1));

    endpoint->connect(server.getAddress());
    REQUIRE(waitForState(State::CONNECTED));
    REQUIRE(waitForChangeTo(State::CONNECTED, 2));

    endpoint->close();
    REQUIRE(endpoint->getState() == State::CLOSED);
    REQUIRE(waitForChangeTo(State::CLOSED, 1));

    // The listeners capture local variables.
    endpoint->onStateChange() -= listenerId1;
    endpoint->removeStateChangeListener(listenerId2);
}

namespace {

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

    StateChanges<dxfc_dxendpoint_state_t> changes{};
    dxfc_dxendpoint_t endpoint{};

    REQUIRE(dxfc_dxendpoint_builder_build(builder, &changes, &endpoint) == DXFC_EC_SUCCESS);
    const dxfc_dxendpoint_state_change_listener stateChangeListener = [](dxfc_dxendpoint_state_t oldState,
                                                                          dxfc_dxendpoint_state_t newState,
                                                                          void *userData) {
        static_cast<StateChanges<dxfc_dxendpoint_state_t> *>(userData)->add(oldState, newState);
    };

    REQUIRE(dxfc_dxendpoint_add_state_change_listener(endpoint, stateChangeListener) == DXFC_EC_SUCCESS);

    // The C API has no executor setter: wait for each notification before the next state change.
    const auto waitForChangeTo = [&](dxfc_dxendpoint_state_t newState, std::size_t times) {
        const auto result = test::waitUntil([&] {
            return changes.countChangesTo(newState) == times;
        });

        INFO("state changes: ", changes.toString());
        CHECK(result);

        return result;
    };

    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_NOT_CONNECTED);

    REQUIRE(dxfc_dxendpoint_connect(endpoint, clientAddress.c_str()) == DXFC_EC_SUCCESS);
    REQUIRE(test::waitUntil([&] {
        return cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CONNECTED;
    }));
    REQUIRE(waitForChangeTo(DXFC_DXENDPOINT_STATE_CONNECTED, 1));

    REQUIRE(dxfc_dxendpoint_disconnect(endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_NOT_CONNECTED);
    REQUIRE(waitForChangeTo(DXFC_DXENDPOINT_STATE_NOT_CONNECTED, 1));

    REQUIRE(dxfc_dxendpoint_connect(endpoint, clientAddress.c_str()) == DXFC_EC_SUCCESS);
    REQUIRE(test::waitUntil([&] {
        return cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CONNECTED;
    }));
    REQUIRE(waitForChangeTo(DXFC_DXENDPOINT_STATE_CONNECTED, 2));

    // Remove the listener before close(): a notification can still arrive after dxfc_dxendpoint_free() and would
    // use the dangling userData (static analysis report, API-14; fixed in the lifetime ticket).
    REQUIRE(dxfc_dxendpoint_remove_state_change_listener(endpoint, stateChangeListener) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_close(endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_CLOSED);

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