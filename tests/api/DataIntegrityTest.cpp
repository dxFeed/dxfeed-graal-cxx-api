// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfeed_graal_cpp_api/internal/detail/Formatter.hpp>

#include <doctest.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

using namespace dxfcpp;
using namespace std::literals;

using stringOpt = std::optional<std::string>;

struct DataIntegrityTestFixture {
    DXEndpoint::Ptr endpoint{};
    DXFeed::Ptr feed{};
    DXPublisher::Ptr pub{};

    DataIntegrityTestFixture() {
        endpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::LOCAL_HUB)
                       ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                       ->build();
        feed = endpoint->getFeed();
        pub = endpoint->getPublisher();
    }

    ~DataIntegrityTestFixture() {
        endpoint->awaitProcessed();
        endpoint->close();
    }
};

struct DataIntegrityLocalAddressTestFixture {
    DXEndpoint::Ptr pubEndpoint{};
    DXPublisher::Ptr pub{};
    DXEndpoint::Ptr feedEndpoint{};
    DXFeed::Ptr feed{};

    DataIntegrityLocalAddressTestFixture() {
        pubEndpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::PUBLISHER)
                       ->build();
        pubEndpoint->connect(":7766");
        pub = pubEndpoint->getPublisher();
        feedEndpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::FEED)
                       ->build();
        feedEndpoint->connect("127.0.0.1:7766");
        feed = feedEndpoint->getFeed();
    }

    ~DataIntegrityLocalAddressTestFixture() {
        feedEndpoint->close();
        pubEndpoint->close();
    }
};

struct DataIntegrityRemoteTestFixture {
    DXEndpoint::Ptr endpoint{};
    DXFeed::Ptr feed{};

    DataIntegrityRemoteTestFixture() {
        endpoint = DXEndpoint::newBuilder()
                       ->withRole(DXEndpoint::Role::FEED)
                       ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                       ->build();
        endpoint->connect("127.0.0.1:7777");
        feed = endpoint->getFeed();
    }

    ~DataIntegrityRemoteTestFixture() {
        endpoint->awaitProcessed();
        endpoint->close();
    }
};

TEST_CASE_FIXTURE(DataIntegrityTestFixture, "Test Message" * doctest::should_fail() ) {
    auto sub = feed->createSubscription(Message::TYPE);

    auto message = std::make_shared<Message>("TEST", "Attachment2");

    sub->addEventListener<Message>([message = message](const std::vector<std::shared_ptr<Message>> & messages) {
        for (auto&& m : messages) {
            fmt::println("{}", m->toString());

            REQUIRE(message->getAttachment() == m->getAttachment());
        }
    });

    sub->addSymbols("TEST");

    pub->publishEvents(message);

    std::this_thread::sleep_for(2s);
}
