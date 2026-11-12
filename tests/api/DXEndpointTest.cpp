// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
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

        for (auto et : e->getEventTypes()) {
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
            result +=
                std::to_string(static_cast<int>(oldState)) + "->" + std::to_string(static_cast<int>(newState)) + " ";
        }

        return result;
    }
};

} // namespace

TEST_CASE("DXEndpoint::Builder: the state follows connect, disconnect and close") {
    using State = DXEndpoint::State;

    const test::LoopbackTcpServer server{};
    const auto executor = InPlaceExecutor::create();
    // The notifications are delivered only by processAllPendingTasks(): the test decides when they arrive.
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->withExecutor(executor)->build();
    StateChanges<State> changes1{};
    StateChanges<State> changes2{};

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
    const dxfc_dxendpoint_state_change_listener stateChangeListener =
        [](dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState, void *userData) {
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

TEST_CASE("dxfc_dxendpoint_t: invalid UTF-8 is an error code, not an exception") {
    dxfc_dxendpoint_builder_t builder{};

    REQUIRE(dxfc_dxendpoint_new_builder(&builder) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_builder_with_role(builder, DXFC_DXENDPOINT_ROLE_FEED) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_builder_with_property(builder, "dxfeed.test.key", "a\xFF") == DXFC_EC_ERROR);

    int supports{};

    REQUIRE(dxfc_dxendpoint_builder_supports_property(builder, "a\xFF", &supports) == DXFC_EC_ERROR);

    dxfc_dxendpoint_t endpoint{};

    REQUIRE(dxfc_dxendpoint_builder_build(builder, nullptr, &endpoint) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_dxendpoint_user(endpoint, "a\xFF") == DXFC_EC_ERROR);
    REQUIRE(dxfc_dxendpoint_password(endpoint, "a\xFF") == DXFC_EC_ERROR);
    REQUIRE(dxfc_dxendpoint_connect(endpoint, "a\xC3\x28") == DXFC_EC_ERROR);
    REQUIRE(cApiGetState(endpoint) == DXFC_DXENDPOINT_STATE_NOT_CONNECTED);

    REQUIRE(dxfc_dxendpoint_close(endpoint) == DXFC_EC_SUCCESS);
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

TEST_CASE("DXEndpoint::Builder::supportsProperty: a known property is supported, a made-up one is not") {
    const auto builder = DXEndpoint::newBuilder();

    CHECK(builder->supportsProperty(DXEndpoint::DXFEED_ADDRESS_PROPERTY));
    CHECK_FALSE(builder->supportsProperty("dxfeed.no.such.property.for.the.test"));
}

TEST_CASE("DXEndpoint::getEventTypes: a FEED endpoint has event types") {
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();

    for (int i = 0; i < 5; ++i) {
        CHECK_FALSE(endpoint->getEventTypes().empty());
    }

    endpoint->close();
}

TEST_CASE("DXEndpoint: awaitNotConnected and disconnectAndClear of a not connected endpoint do not throw") {
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();

    REQUIRE(endpoint->getState() == DXEndpoint::State::NOT_CONNECTED);
    CHECK_NOTHROW(endpoint->awaitNotConnected());
    CHECK_NOTHROW(endpoint->disconnectAndClear());
    CHECK(endpoint->getState() == DXEndpoint::State::NOT_CONNECTED);

    endpoint->close();
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
TEST_CASE("DXEndpoint: the property keys of QDS 3.330-3.351 as in Java") {
    CHECK(std::string(DXEndpoint::DXFEED_STRIPE_PROPERTY) == "dxfeed.stripe");
    CHECK(std::string(DXEndpoint::DXFEED_STICKY_SUBSCRIPTION_PROPERTY) == "dxfeed.stickySubscriptionPeriod");
    CHECK(std::string(DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY) == "dxfeed.requestedAggregationPeriod");

    const auto builder = DXEndpoint::newBuilder();

    CHECK(builder->supportsProperty(DXEndpoint::DXFEED_STRIPE_PROPERTY));
    CHECK(builder->supportsProperty(DXEndpoint::DXFEED_STICKY_SUBSCRIPTION_PROPERTY));
    CHECK(builder->supportsProperty(DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY));

    // The builder accepts the properties and builds the endpoint with them.
    const auto endpoint = DXEndpoint::newBuilder()
                              ->withRole(DXEndpoint::Role::FEED)
                              ->withProperty(DXEndpoint::DXFEED_STICKY_SUBSCRIPTION_PROPERTY, "1s")
                              ->withProperty(DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY, "0.5s")
                              ->build();

    REQUIRE(endpoint);
    endpoint->close();
}

TEST_CASE("DXFeedSubscription batch limits can be bound by reference") {
    // Binds the constants by reference (an ODR use): links only when they are defined (static constexpr).
    const auto &optimal = DXFeedSubscription::OPTIMAL_BATCH_LIMIT;
    const auto &max = DXFeedSubscription::MAX_BATCH_LIMIT;

    CHECK(optimal == 0);
    CHECK(max == std::numeric_limits<std::int32_t>::max());
    CHECK(std::max(DXFeedSubscription::OPTIMAL_BATCH_LIMIT, DXFeedSubscription::MAX_BATCH_LIMIT) == max);
}

namespace {

// A kind of invalid UTF-8 after a valid prefix: the suffix and the bytes the error names.
struct InvalidUtf8 {
    const char *kind;
    const char *suffix;
    const char *bytes;
};

const std::vector<InvalidUtf8> INVALID_UTF8 = {
    {"a stray byte", "\xFF", "0xFF"},
    {"a broken sequence", "\xC3\x28", "0xC3 0x28"},
    {"a truncated sequence", "\xE2\x82", "0xE2 0x82"},
    {"an overlong form", "\xC0\xAF", "0xC0 0xAF"},
    {"an encoded surrogate", "\xED\xA0\x80", "0xED 0xA0 0x80"},
};

// Cyrillic and an emoji (U+1F600).
constexpr auto VALID_MULTIBYTE = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xF0\x9F\x98\x80";

} // namespace

TEST_CASE("DXEndpoint: an API parameter with invalid UTF-8 throws with the place, the offset and the bytes") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    for (const auto &[kind, suffix, bytes] : INVALID_UTF8) {
        CAPTURE(kind);
        const auto expected = "DXEndpoint::connect(address): invalid UTF-8 at byte 4: "s + bytes;

        CHECK_THROWS_WITH_AS(endpoint->connect("demo"s + suffix), expected.c_str(), InvalidArgumentException);
    }

    CHECK(endpoint->getState() == DXEndpoint::State::NOT_CONNECTED);
    endpoint->close();
}

TEST_CASE("DXEndpoint: a secret with invalid UTF-8 throws with the place and the offset, without the bytes") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    for (const auto &[kind, suffix, bytes] : INVALID_UTF8) {
        CAPTURE(kind);
        CHECK_THROWS_WITH_AS(endpoint->password("p"s + suffix),
                             "DXEndpoint::password(password): invalid UTF-8 at byte 1", InvalidArgumentException);
        CHECK_THROWS_WITH_AS(endpoint->user("u"s + suffix), "DXEndpoint::user(user): invalid UTF-8 at byte 1",
                             InvalidArgumentException);
    }

    CHECK_NOTHROW(endpoint->user(VALID_MULTIBYTE));
    CHECK_NOTHROW(endpoint->password(VALID_MULTIBYTE));
    endpoint->close();
}

TEST_CASE("DXEndpoint::Builder::withProperty: invalid UTF-8 throws, the user and the password without the bytes") {
    const auto builder = DXEndpoint::newBuilder();

    CHECK_THROWS_WITH_AS(builder->withProperty(DXEndpoint::DXFEED_PASSWORD_PROPERTY, "p\xC3\x28"),
                         "DXEndpoint::Builder::withProperty(value): invalid UTF-8 at byte 1", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(builder->withProperty(DXEndpoint::DXFEED_USER_PROPERTY, "u\xFF"),
                         "DXEndpoint::Builder::withProperty(value): invalid UTF-8 at byte 1", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(builder->withProperty("custom.property", "v\xFF"),
                         "DXEndpoint::Builder::withProperty(value): invalid UTF-8 at byte 1: 0xFF",
                         InvalidArgumentException);
    CHECK_THROWS_WITH_AS(builder->withProperty("custom\xFF", "v"),
                         "DXEndpoint::Builder::withProperty(key): invalid UTF-8 at byte 6: 0xFF",
                         InvalidArgumentException);

    builder->withProperty("custom.property", VALID_MULTIBYTE);
    builder->build()->close();
}
