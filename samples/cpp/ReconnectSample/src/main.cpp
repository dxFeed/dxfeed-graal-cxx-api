// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <mutex>
#include <string>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

// Demonstrates how to connect to an endpoint, subscribe to market data events,
// handle reconnections and re-subscribing.
int main() {
    try {
        const auto address = "demo.dxfeed.com:7300"; // The address of the DxFeed endpoint.
        const auto symbol = "ETH/USD:GDAX"; // The symbol for which we want to receive quotes.

        // Disable QD logging.
        // Logging::init();

        // Create new endpoint and add a listener for state changes.
        auto endpoint = DXEndpoint::create();

        endpoint->addStateChangeListener([](DXEndpoint::State oldState, DXEndpoint::State newState) {
            std::cout << fmt::format("Connection state changed: {}->{}", DXEndpoint::stateToString(oldState),
                                     DXEndpoint::stateToString(newState))
                      << std::endl;
        });

        // Connect to the endpoint using the specified address.
        endpoint->connect(address);

        // Create a subscription for Quote events.
        auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

        subscription->addEventListener<Quote>([](const auto &quotes) {
            // Event listener that prints each received event.
            for (const auto &quote : quotes) {
                std::cout << quote << std::endl;
            }
        });

        // Add the specified symbol to the subscription.
        subscription->addSymbols(symbol);

        // Wait for five seconds to allow some quotes to be received.
        std::this_thread::sleep_for(5s);

        // Disconnect from the endpoint.
        endpoint->disconnect();

        // Wait for another five seconds to ensure quotes stop coming in.
        std::this_thread::sleep_for(5s);

        // Reconnect to the endpoint.
        // The subscription is automatically re-subscribed, and quotes start coming into the listener again.
        // Another address can also be passed on.
        endpoint->connect(address);

        // Keep the application running indefinitely.
        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}