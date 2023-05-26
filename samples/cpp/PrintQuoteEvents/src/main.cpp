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

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#    define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

int main() {
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        using namespace dxfcpp::literals;

        auto s = dxfcpp::StringSymbol("123123");
        s.toGraal();

        //dxfcpp::System::setProperty("dxfeed.wildcard.enable", "true");
        auto builder = dxfcpp::DXEndpoint::newBuilder()
                           ->withRole(dxfcpp::DXEndpoint::Role::FEED)
                           ->withProperty(dxfcpp::DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true");
        auto endpoint = builder->build();

        auto sub = endpoint->getFeed()->createSubscription({dxfcpp::EventTypeEnum::QUOTE});

        sub->addEventListener([](auto &&events) {
            for (const auto &e : events) {
                std::cout << e << std::endl;
            }
        });

        sub->addSymbol("AAPL");
        sub->addSymbol("IBM"sv);
        sub->addSymbol("TSLA"s);
        sub->addSymbol(dxfcpp::WildcardSymbol::ALL);
        sub->addSymbol("AMD"_s);
        sub->addSymbol("*"_wcs);

        endpoint->connect("demo.dxfeed.com:7300");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        sub->removeSymbol("TSLA");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        endpoint->close();
    }
}
