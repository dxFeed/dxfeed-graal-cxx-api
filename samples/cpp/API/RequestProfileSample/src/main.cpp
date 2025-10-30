// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>
#include <iostream>
#include <string>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

/*
 * Using address like "localhost:7700" and a symbol list "A:TEST" it connects to the running PublishProfilesSample,
 * prints symbol profile event, and exits.
 */
int main(int argc, char *argv[]) {
    try {
        if (argc < 3) {
            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        // Parse args.
        const std::string address = argv[1];
        const std::string symbol = argv[2];
        const auto endpoint = DXEndpoint::create(DXEndpoint::Role::FEED)->connect(address);
        const auto profile = endpoint->getFeed()->getLastEventPromise<Profile>(symbol)->await(5s);

        std::cout << profile << '\n';
        endpoint->close();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}