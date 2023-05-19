// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <filesystem>

auto cApiStateToString(dxfc_dxendpoint_state_t state) {
    switch (state) {
    case DXFC_DXENDPOINT_STATE_NOT_CONNECTED:
        return "NOT_CONNECTED";
    case DXFC_DXENDPOINT_STATE_CONNECTING:
        return "CONNECTING";
    case DXFC_DXENDPOINT_STATE_CONNECTED:
        return "CONNECTED";
    case DXFC_DXENDPOINT_STATE_CLOSED:
        return "CLOSED";
    }

    return "";
}

int main() {
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;

        auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
        auto endpoint = builder->build();

        endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
            dxfcpp::debug("{}", std::string("State changed: ") + dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
                                    dxfcpp::DXEndpoint::stateToString(newState));
        };

        auto sub =
            endpoint->getFeed()->createSubscription({dxfcpp::EventTypeEnum::QUOTE});

        sub->addEventListener([](auto &&events) {
            for (const auto &e : events) {
                std::cout << e << std::endl;
            }
        });

        sub->addSymbol("AAPL");
        sub->addSymbol("IBM"sv);
        sub->addSymbol("TSLA"s);

        endpoint->connect("demo.dxfeed.com:7300");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        sub->removeSymbol("TSLA");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        endpoint->close();
    }
}
