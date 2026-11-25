// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

using namespace dxfcpp;
using namespace std::literals;

void printUsage() {
    const auto usageString = R"(
Usage:
ReconnectSample [<address>] [--duration <seconds>]

Where:
    address  - The address to connect to (default: "demo.dxfeed.com:7300"). A list of addresses
               "host1:port1,host2:port2" is one connection that moves to the next address after a failure;
               [connectOrder=ordered] keeps the order of the list (the default is shuffle),
               [reconnectDelay=<ms>] sets the delay before an attempt (the default is 10 seconds).
    duration - Run for the given number of seconds and exit: the exit code is 0 if the endpoint was connected,
               1 otherwise. Without it the sample disconnects after 5 seconds, connects again after another 5 seconds
               and runs until enter is pressed.

Examples:
    ReconnectSample
    ReconnectSample "127.0.0.1:1,demo.dxfeed.com:7300[connectOrder=ordered]" --duration 30
)";

    std::cout << usageString << std::endl;
}

std::string timestamp() {
    return TimeFormat::DEFAULT_WITH_MILLIS.format(now());
}

// Prints the state of a connector, the addresses it is connected to and its last connection error.
void printConnector(const MessageConnectorInfo &connector) {
    std::string line =
        timestamp() + " Connector " + connector.address + ": " + MessageConnectorInfo::stateToString(connector.state);

    for (const auto &currentAddress : connector.currentAddresses) {
        line += ", connected to " + currentAddress;
    }

    if (connector.lastError) {
        line += "\n    last error: " + connector.lastError->toString();
    }

    std::cout << line + "\n" << std::flush;
}

// Demonstrates how to connect to an endpoint, subscribe to market data events, handle reconnections and
// re-subscribing, and log the state of the connection with the last connection error.
int main(int argc, char *argv[]) {
    try {
        std::string address = "demo.dxfeed.com:7300"; // The address of the DxFeed endpoint.
        const auto symbol = "ETH/USD:GDAX";           // The symbol for which we want to receive quotes.
        std::optional<std::chrono::duration<double>> duration{};

        for (int i = 1; i < argc; i++) {
            const std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                printUsage();

                return 0;
            }

            if (arg == "--duration") {
                if (i + 1 == argc) {
                    std::cerr << "No value of --duration\n";
                    printUsage();

                    return 1;
                }

                duration = std::chrono::duration<double>(std::stod(argv[++i]));
            } else if (i == 1) {
                address = arg;
            } else {
                std::cerr << "Unexpected argument: " << arg << '\n';
                printUsage();

                return 1;
            }
        }

        // Disable QD logging.
        // Logging::init();

        // Create a new endpoint and add a listener for state changes.
        const auto endpoint = DXEndpoint::create();
        const auto wasConnected = std::make_shared<std::atomic<bool>>(false);

        endpoint->addStateChangeListener([wasConnected](DXEndpoint::State oldState, DXEndpoint::State newState) {
            if (newState == DXEndpoint::State::CONNECTED) {
                *wasConnected = true;
            }

            std::cout << timestamp() + " Endpoint state: " + DXEndpoint::stateToString(oldState) + " -> " +
                             DXEndpoint::stateToString(newState) + "\n"
                      << std::flush;
        });

        // A failed attempt does not change the state of the endpoint: the connector listener tells which address
        // failed and why (the category of the error and the message of the exception).
        endpoint->addMessageConnectorListener(printConnector);

        // Connect to the endpoint using the specified address.
        endpoint->connect(address);

        // The listener does not receive the current states when it is added: print them once.
        for (const auto &connector : endpoint->getConnectors()) {
            printConnector(connector);
        }

        // Create a subscription for Quote events.
        const auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

        subscription->addEventListener<Quote>([](const auto &quotes) {
            // Event listener that prints each received event.
            for (const auto &quote : quotes) {
                std::cout << quote->toString() + "\n";
            }
        });

        // Add the specified symbol to the subscription.
        subscription->addSymbols(symbol);

        if (duration) {
            std::this_thread::sleep_for(*duration);
            endpoint->closeAndAwaitTermination();

            return *wasConnected ? 0 : 1;
        }

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

        // Keep the application running until enter is pressed.
        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    } catch (const std::exception &e) {
        // For example, std::stod of an invalid number.
        std::cerr << e.what() << '\n';

        return 1;
    }

    return 0;
}
