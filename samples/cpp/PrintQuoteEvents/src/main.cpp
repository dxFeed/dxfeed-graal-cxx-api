// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxFeedNativeCppAPI.hpp>

int main() {
    {
        // dxfcpp::DXFeed::getInstance();
//        auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
//        auto endpoint = builder->build();

        auto endpoint = dxfcpp::DXEndpoint::getInstance(dxfcpp::DXEndpoint::Role::FEED);

        endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
            dxfcpp::detail::debug("{}", std::string("State changed: ") + dxfcpp::DXEndpoint::stateToString(oldState) +
                                            " -> " + dxfcpp::DXEndpoint::stateToString(newState));
        };

        endpoint->connect("demo.dxfeed.com:7300");

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cin.get();
}