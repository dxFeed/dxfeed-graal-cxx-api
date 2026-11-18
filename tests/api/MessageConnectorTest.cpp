// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

namespace {

bool contains(const std::string &text, const std::string &part) {
    return text.find(part) != std::string::npos;
}

// The snapshots that a connector listener got, for the checks of the test thread.
struct Snapshots {
    mutable std::mutex mutex{};
    std::vector<MessageConnectorInfo> values{};

    void add(const MessageConnectorInfo &info) {
        std::lock_guard lock{mutex};

        values.push_back(info);
    }

    template <typename Predicate> std::optional<MessageConnectorInfo> find(Predicate &&predicate) const {
        std::lock_guard lock{mutex};

        const auto found = std::find_if(values.begin(), values.end(), predicate);

        return found == values.end() ? std::nullopt : std::optional{*found};
    }

    std::size_t size() const {
        std::lock_guard lock{mutex};

        return values.size();
    }
};

// A refused connect to a closed loopback port takes up to a few seconds on Windows.
constexpr auto CONNECT_FAILURE_TIMEOUT = 30s;

std::string closedLoopbackAddress() {
    return "127.0.0.1:" + std::to_string(test::findFreeLoopbackPort());
}

} // namespace

TEST_CASE("MessageConnector: a new endpoint has no connectors and no connection error") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    CHECK(endpoint->getConnectors().empty());
    CHECK_FALSE(endpoint->getLastConnectionError().has_value());

    endpoint->close();
}

TEST_CASE("MessageConnector: a connected endpoint has one CONNECTED client socket connector") {
    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    endpoint->connect(server.getAddress());

    std::vector<MessageConnectorInfo> connectors{};

    // The state, the addresses and the statistics of a connector are read one after another, not as one snapshot: the
    // state may already be CONNECTED while the address of the new connection is not yet listed.
    REQUIRE(test::waitUntil([&] {
        connectors = endpoint->getConnectors();

        return connectors.size() == 1 && connectors[0].state == MessageConnectorInfo::State::CONNECTED &&
               connectors[0].connectionCount == 1 && !connectors[0].currentAddresses.empty() &&
               connectors[0].statistics.activeConnectionCount == 1;
    }));

    const auto &connector = connectors[0];

    INFO(connector.toString());
    CHECK(connector.kind == MessageConnectorInfo::Kind::CLIENT_SOCKET);
    CHECK(connector.address == server.getAddress());
    CHECK(connector.active);
    CHECK_FALSE(connector.name.empty());
    REQUIRE(connector.currentAddresses.size() == 1);
    CHECK(contains(connector.currentAddresses[0], server.getAddress().substr(server.getAddress().rfind(':'))));
    CHECK(connector.statistics.activeConnectionCount == 1);
    CHECK_FALSE(connector.lastError.has_value());
    CHECK(contains(connector.toString(), "CONNECTED"));
    CHECK(contains(connector.toString(), "CLIENT_SOCKET"));
    CHECK(contains(connector.toString(), server.getAddress()));

    endpoint->close();

    CHECK(endpoint->getConnectors().empty());
}

TEST_CASE("MessageConnector: a closed port is a CONNECT_FAILED error with the address") {
    const auto address = closedLoopbackAddress();
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

    endpoint->connect(address);

    std::optional<ConnectionError> error{};

    REQUIRE(test::waitUntil(
        [&] {
            error = endpoint->getLastConnectionError();

            return error.has_value();
        },
        CONNECT_FAILURE_TIMEOUT));

    INFO(error->toString());
    CHECK(error->category == ConnectionError::Category::CONNECT_FAILED);
    CHECK(contains(error->address, address));
    CHECK_FALSE(error->dropped);
    CHECK(error->count >= 1);
    CHECK(error->time > 0);
    CHECK(contains(error->toString(), "CONNECT_FAILED"));
    CHECK(contains(error->toString(), address));

    // The connector keeps its error: the endpoint stays connecting between the attempts.
    const auto connectors = endpoint->getConnectors();

    REQUIRE(connectors.size() == 1);
    REQUIRE(connectors[0].lastError.has_value());
    CHECK(connectors[0].lastError->category == ConnectionError::Category::CONNECT_FAILED);
    CHECK(connectors[0].state != MessageConnectorInfo::State::CONNECTED);

    endpoint->close();

    // The error of the endpoint stays after the close.
    CHECK(endpoint->getLastConnectionError().has_value());
}

TEST_CASE("MessageConnector: the listener gets the CONNECTED snapshot") {
    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto snapshots = std::make_shared<Snapshots>();

    endpoint->addMessageConnectorListener([snapshots](const MessageConnectorInfo &info) {
        snapshots->add(info);
    });
    endpoint->connect(server.getAddress());

    REQUIRE(test::waitUntil([&] {
        return snapshots
            ->find([](const MessageConnectorInfo &info) {
                return info.state == MessageConnectorInfo::State::CONNECTED;
            })
            .has_value();
    }));

    const auto connected = snapshots->find([](const MessageConnectorInfo &info) {
        return info.state == MessageConnectorInfo::State::CONNECTED;
    });

    CHECK(connected->address == server.getAddress());
    CHECK(connected->kind == MessageConnectorInfo::Kind::CLIENT_SOCKET);

    endpoint->close();
}

TEST_CASE("MessageConnector: the listener reads the error of a failed connect already recorded") {
    const auto address = closedLoopbackAddress();
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto snapshots = std::make_shared<Snapshots>();
    const auto endpointErrorSeen = std::make_shared<std::atomic<bool>>(false);

    endpoint->addMessageConnectorListener(
        [snapshots, endpointErrorSeen, weakEndpoint = std::weak_ptr{endpoint}](const MessageConnectorInfo &info) {
            snapshots->add(info);

            if (info.lastError) {
                // The error of the endpoint is recorded with the error of the connector.
                if (const auto e = weakEndpoint.lock(); e && e->getLastConnectionError()) {
                    *endpointErrorSeen = true;
                }
            }
        });
    endpoint->connect(address);

    REQUIRE(test::waitUntil(
        [&] {
            return snapshots
                ->find([](const MessageConnectorInfo &info) {
                    return info.lastError.has_value();
                })
                .has_value();
        },
        CONNECT_FAILURE_TIMEOUT));

    const auto failed = snapshots->find([](const MessageConnectorInfo &info) {
        return info.lastError.has_value();
    });

    CHECK(failed->lastError->category == ConnectionError::Category::CONNECT_FAILED);
    CHECK_FALSE(failed->lastError->dropped);
    CHECK(endpointErrorSeen->load());

    endpoint->close();
}

TEST_CASE("MessageConnector: an exception of a listener goes to ErrorManager, the other listeners are called") {
    const test::ErrorManagerGuard errorManagerGuard{};
    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto snapshots = std::make_shared<Snapshots>();

    endpoint->addMessageConnectorListener([](const MessageConnectorInfo &) {
        throw std::runtime_error("connector listener failure");
    });
    endpoint->addMessageConnectorListener([snapshots](const MessageConnectorInfo &info) {
        snapshots->add(info);
    });
    endpoint->connect(server.getAddress());

    const auto reported = [] {
        const auto errors = ErrorManager::getErrors();

        return std::any_of(errors.begin(), errors.end(), [](const ErrorInfo &error) {
            return error.level == ErrorInfo::Level::ERROR && error.component == "DXEndpoint" &&
                   contains(error.message, "connector listener failure");
        });
    };

    REQUIRE(test::waitUntil([&] {
        return reported() && snapshots->size() > 0;
    }));

    endpoint->close();
}

TEST_CASE("MessageConnector: a removed listener is not called") {
    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto kept = std::make_shared<Snapshots>();
    const auto removed = std::make_shared<Snapshots>();

    endpoint->addMessageConnectorListener([kept](const MessageConnectorInfo &info) {
        kept->add(info);
    });

    const auto removedId = endpoint->addMessageConnectorListener([removed](const MessageConnectorInfo &info) {
        removed->add(info);
    });

    endpoint->removeMessageConnectorListener(removedId);
    endpoint->removeMessageConnectorListener(removedId); // a second removal changes nothing
    endpoint->connect(server.getAddress());

    REQUIRE(test::waitUntil([&] {
        return kept
            ->find([](const MessageConnectorInfo &info) {
                return info.state == MessageConnectorInfo::State::CONNECTED;
            })
            .has_value();
    }));

    endpoint->close();

    CHECK(removed->size() == 0);
}

TEST_CASE("MessageConnector: a connection closed by the server is a dropped error read in the listener") {
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    const auto snapshots = std::make_shared<Snapshots>();

    endpoint->addMessageConnectorListener([snapshots](const MessageConnectorInfo &info) {
        snapshots->add(info);
    });

    {
        const test::LoopbackTcpServer server{};

        endpoint->connect(server.getAddress());

        REQUIRE(test::waitUntil([&] {
            return snapshots
                ->find([](const MessageConnectorInfo &info) {
                    return info.state == MessageConnectorInfo::State::CONNECTED;
                })
                .has_value();
        }));

        // The server closes its connections when it is destroyed.
    }

    REQUIRE(test::waitUntil(
        [&] {
            return snapshots
                ->find([](const MessageConnectorInfo &info) {
                    return info.lastError && info.lastError->dropped;
                })
                .has_value();
        },
        CONNECT_FAILURE_TIMEOUT));

    const auto dropped = endpoint->getLastConnectionError();

    REQUIRE(dropped.has_value());
    INFO(dropped->toString());
    CHECK(dropped->count >= 1);

    endpoint->close();
}

TEST_CASE("MessageConnector: a listener that holds its endpoint does not keep it alive after close()") {
    std::weak_ptr<DXEndpoint> weakEndpoint{};

    {
        const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);

        weakEndpoint = endpoint;
        endpoint->addMessageConnectorListener([endpoint](const MessageConnectorInfo &) {
        });
        endpoint->close();
    }

    CHECK(test::waitUntil([&] {
        return weakEndpoint.expired();
    }));
}

TEST_CASE("MessageConnector: a listener that adds a listener while the endpoint is closed does not deadlock") {
    const test::LoopbackTcpServer server{};
    const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    // Shared with the listener: it may outlive the test if the test fails.
    const auto entered = std::make_shared<std::atomic<bool>>(false);
    const auto added = std::make_shared<std::atomic<bool>>(false);

    endpoint->addMessageConnectorListener(
        [entered, added, weakEndpoint = std::weak_ptr{endpoint}](const MessageConnectorInfo &) {
            if (entered->exchange(true)) {
                return;
            }

            // The close on the test thread runs meanwhile.
            std::this_thread::sleep_for(200ms);

            if (const auto e = weakEndpoint.lock()) {
                e->addMessageConnectorListener([](const MessageConnectorInfo &) {
                });
                *added = true;
            }
        });
    endpoint->connect(server.getAddress());

    REQUIRE(test::waitUntil([&] {
        return entered->load();
    }));

    // The release of the listener of the SDK waits for the call in progress, which takes the lock of the endpoint. The
    // release may run on the thread of the CLOSED notification instead, so close() itself may return before the call.
    endpoint->close();

    CHECK(test::waitUntil([&] {
        return added->load();
    }));
}

TEST_CASE("ConnectionError::toString: the time in UTC, the category and the optional parts") {
    ConnectionError error{};

    error.time = 1'767'323'045'678; // 2026-01-02T03:04:05.678Z
    error.address = "127.0.0.1:1";
    error.category = ConnectionError::Category::TLS;
    error.dropped = true;
    error.count = 3;

    const auto text = error.toString();

    CHECK(contains(text, "2026-01-02T03:04:05.678Z"));
    CHECK(contains(text, "category = TLS"));
    CHECK(contains(text, "dropped = true"));
    CHECK(contains(text, "address = 127.0.0.1:1"));
    CHECK(contains(text, "count = 3"));
    CHECK_FALSE(contains(text, "exception ="));
    CHECK_FALSE(contains(text, "message ="));

    error.time = -1;
    error.exceptionClassName = "java.net.ConnectException";
    error.message = "Connection refused\nline 2";

    const auto withException = error.toString();

    CHECK(contains(withException, "1969-12-31T23:59:59.999Z"));
    CHECK(contains(withException, "exception = java.net.ConnectException"));
    CHECK(contains(withException, "message = Connection refused line 2"));

    CHECK(std::string{ConnectionError::categoryToString(ConnectionError::Category::HEARTBEAT_TIMEOUT)} ==
          "HEARTBEAT_TIMEOUT");
    CHECK(std::string{MessageConnectorInfo::kindToString(MessageConnectorInfo::Kind::SERVER_SOCKET)} ==
          "SERVER_SOCKET");
    CHECK(std::string{MessageConnectorInfo::stateToString(MessageConnectorInfo::State::DISCONNECTED)} ==
          "DISCONNECTED");
}
