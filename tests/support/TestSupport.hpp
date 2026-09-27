// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace dxfcpp::test {

using namespace std::literals;

/**
 * Polls the predicate until it returns `true` or the timeout expires.
 *
 * @return The last value of the predicate.
 */
template <typename Predicate>
bool waitUntil(Predicate &&predicate, std::chrono::milliseconds timeout = 10s,
               std::chrono::milliseconds pollInterval = 10ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return predicate();
        }

        std::this_thread::sleep_for(pollInterval);
    }

    return true;
}

/**
 * A TCP server on a free loopback port that accepts connections and never sends anything. A client endpoint connected
 * to getAddress() reaches the CONNECTED state without external network. The server listens only on 127.0.0.1, so it
 * does not trigger firewall prompts. To test the data transfer, use a PUBLISHER endpoint on
 * `:<findFreeLoopbackPort()>[bindAddr=127.0.0.1]` instead (the `bindAddr` property needs Graal SDK v3.5.0+).
 */
class LoopbackTcpServer final {
    std::intptr_t listenSocket_{};
    std::uint16_t port_{};
    std::thread acceptThread_{};
    mutable std::mutex mutex_{};
    bool isStopped_{};
    std::vector<std::intptr_t> clientSockets_{};

  public:
    LoopbackTcpServer();
    ~LoopbackTcpServer();

    LoopbackTcpServer(const LoopbackTcpServer &) = delete;
    LoopbackTcpServer &operator=(const LoopbackTcpServer &) = delete;

    /// Returns the address for DXEndpoint::connect(), for example `127.0.0.1:50123`.
    std::string getAddress() const;

    std::size_t getAcceptedConnectionCount() const;
};

/**
 * Returns a loopback TCP port that is free at the moment of the call. Another process may take it before it is used;
 * prefer LoopbackTcpServer where a server that only accepts connections is enough.
 */
std::uint16_t findFreeLoopbackPort();

/**
 * A `LOCAL_HUB` endpoint with an in-place executor: events published with publishAndProcess() are delivered to the
 * listeners of the feed in the calling thread before the method returns. No network is used.
 */
struct LocalHubFixture {
    std::shared_ptr<InPlaceExecutor> executor;
    std::shared_ptr<DXEndpoint> endpoint;
    std::shared_ptr<DXFeed> feed;
    std::shared_ptr<DXPublisher> publisher;

    explicit LocalHubFixture(const std::unordered_map<std::string, std::string> &properties = {})
        : executor(InPlaceExecutor::create()), endpoint(createEndpoint(executor, properties)),
          feed(endpoint->getFeed()), publisher(endpoint->getPublisher()) {
    }

    // The executor must be set before the feed is obtained: the feed keeps the executor it was created with, and
    // DXEndpoint::executor() shuts down the default one (tasks are then rejected with RejectedExecutionException).
    static std::shared_ptr<DXEndpoint> createEndpoint(const std::shared_ptr<InPlaceExecutor> &executor,
                                                      const std::unordered_map<std::string, std::string> &properties) {
        auto builder = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::LOCAL_HUB);

        for (const auto &[key, value] : properties) {
            builder = builder->withProperty(key, value);
        }

        auto endpoint = builder->build();

        endpoint->executor(executor);

        return endpoint;
    }

    ~LocalHubFixture() {
        endpoint->close();
    }

    void process() const {
        executor->processAllPendingTasks();
    }

    template <typename Events> void publishAndProcess(const Events &events) const {
        publisher->publishEvents(events);
        process();
    }

    template <typename E> void publishAndProcess(const std::shared_ptr<E> &event) const {
        publisher->publishEvents(event);
        process();
    }
};

} // namespace dxfcpp::test

// Lets doctest print endpoint states in failed assertions (MSVC does not print scoped enums otherwise).
template <> struct doctest::StringMaker<dxfcpp::DXEndpoint::State> {
    static String convert(dxfcpp::DXEndpoint::State state) {
        return dxfcpp::DXEndpoint::stateToString(state).c_str();
    }
};
