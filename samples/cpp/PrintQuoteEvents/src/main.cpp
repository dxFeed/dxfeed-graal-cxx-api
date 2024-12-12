// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

/*
 * A simple sample that shows how to subscribe to quotes for one instrument,
 * and print all received quotes to the console.
 * Use default DXFeed instance for that data feed address is defined by "dxfeed.properties" file.
 * The properties file is copied to the build output directory from the project directory.
 */
int main(int argc, char *argv[]) {
    using namespace dxfcpp;

    try {
        // Specified instrument name, for example AAPL, IBM, MSFT, etc.
        auto symbol = argc > 1 ? argv[1] : "AAPL";

        // Creates a subscription attached to a default DXFeed with a Quote event type.
        // The endpoint address to use is stored in the "dxfeed.properties" file.
        auto subscription = DXFeed::getInstance()->createSubscription(Quote::TYPE);

        // Listener must be attached before symbols are added.
        subscription->addEventListener([](const auto &events) {
            // Prints all received events.
            for (const auto &e : events) {
                std::cout << e << "\n";
            }
        });

        // Adds specified symbol.
        subscription->addSymbols(symbol);

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
