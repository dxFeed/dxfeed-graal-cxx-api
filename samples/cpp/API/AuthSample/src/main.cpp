// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <random>
#include <string>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

std::random_device rd;  // a seed source for the random number engine
std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()

// Demonstrates how to connect to endpoint requires authentication token, subscribe to market data events, and handle
// periodic token updates.
int main() {
    try {
        const auto address = "demo.dxfeed.com:7300";

        // Disable QD logging.
        // Logging::init();

        auto generateToken = [] {
            std::uniform_int_distribution<std::size_t> lengthDistrib(4u, 9u);
            std::uniform_int_distribution<> letterDistrib('a', 'z');

            std::string token(lengthDistrib(gen), ' ');

            for (char &i : token) {
                i = static_cast<char>(letterDistrib(gen));
            }

            return token;
        };

        // Add a listener for state changes to the default application-wide singleton instance of DXEndpoint.
        DXEndpoint::getInstance()->addStateChangeListener([](DXEndpoint::State oldState, DXEndpoint::State newState) {
            std::cout << fmt::format("Connection state changed: {}->{}", DXEndpoint::stateToString(oldState),
                                     DXEndpoint::stateToString(newState))
                      << std::endl;
        });

        // Set up a timer to periodically update the token and reconnect every 10 seconds.
        // The first connection will be made immediately.
        // After reconnection, all existing subscriptions will be re-subscribed automatically.
        auto updateTokenTimer = Timer::schedule(
            [address, &generateToken] {
                DXEndpoint::getInstance()->connect(fmt::format("{}[login=entitle:{}]", address, generateToken()));
            },
            0, 10s);

        // Create a subscription for Quote events.
        const auto subscription = DXFeed::getInstance()->createSubscription(Quote::TYPE);

        subscription->addEventListener<Quote>([](const auto &quotes) {
            // Event listener that prints each received event.
            for (const auto &quote : quotes) {
                std::cout << quote << std::endl;
            }
        });

        // Add the specified symbol to the subscription.
        subscription->addSymbols("ETH/USD:GDAX");

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}