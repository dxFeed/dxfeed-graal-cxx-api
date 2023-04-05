// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <api.hpp>
#include "../include/dxfeed_graal_native_c_api/api.h"

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
    dxfc_dxendpoint_builder_t builder{};

    auto result = dxfc_dxendpoint_new_builder(&builder);

    if (result != DXFC_EC_SUCCESS) {
        return 2;
    }

    result = dxfc_dxendpoint_builder_with_role(builder, DXFC_DXENDPOINT_ROLE_FEED);

    if (result != DXFC_EC_SUCCESS) {
        return 3;
    }

    dxfc_dxendpoint_t endpoint{};

    result = dxfc_dxendpoint_builder_build(builder, nullptr, &endpoint);

    if (result != DXFC_EC_SUCCESS) {
        return 4;
    }

    result = dxfc_dxendpoint_add_state_change_listener(
        endpoint, [](dxfc_dxendpoint_state_t oldState, dxfc_dxendpoint_state_t newState, void *) {
            dxfcpp::debug("dxfc_dxendpoint_builder_t Test: {}", std::string("State changed: ") +
                                                                    cApiStateToString(oldState) + " -> " +
                                                                    cApiStateToString(newState));
        });

    if (result != DXFC_EC_SUCCESS) {
        return 5;
    }

    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");

    if (result != DXFC_EC_SUCCESS) {
        return 6;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    result = dxfc_dxendpoint_disconnect(endpoint);

    if (result != DXFC_EC_SUCCESS) {
        return 7;
    }

    result = dxfc_dxendpoint_connect(endpoint, "demo.dxfeed.com:7300");

    if (result != DXFC_EC_SUCCESS) {
        return 8;
    }

    dxfc_dxendpoint_close(endpoint);


//    auto s0 = dxfcpp::DXFeedSubscription::create(dxfcpp::EventTypeEnum::QUOTE);
//    auto s1 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE});
//    auto s2 = dxfcpp::DXFeedSubscription::create({dxfcpp::EventTypeEnum::QUOTE, dxfcpp::EventTypeEnum::CANDLE});
//
//    dxfcpp::DXFeed::getInstance()->attachSubscription(s2);
//    dxfcpp::DXFeed::getInstance();
//
//    std::cout << fmt::format("{:#06x}", 123) << std::endl;
//    std::cout << fmt::format("{:#x}", 123) << std::endl;
//
//    std::cout << dxfcpp::ShortSaleRestriction::ACTIVE << std::endl;
//    std::cout << dxfcpp::ShortSaleRestriction::valueOf(100500) << std::endl;
//    std::cout << dxfcpp::PriceType::valueOf(100500) << std::endl;
//
//    dxfcpp::Quote q{};
//
//    std::cout << dxfcpp::nowStrWithTimeZone() << std::endl;
//    std::cout << dxfcpp::formatTimeStamp(1677448613000) << std::endl;
//    std::cout << dxfcpp::formatTimeStampWithMillis(1677448613023) << std::endl;
//
//    {
//        auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
//        auto endpoint = builder->build();
//
//        endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
//            dxfcpp::debug("{}", std::string("State changed: ") + dxfcpp::DXEndpoint::stateToString(oldState) + " -> " +
//                                    dxfcpp::DXEndpoint::stateToString(newState));
//        };
//
//        auto sub =
//            endpoint->getFeed()->createSubscription({dxfcpp::EventTypeEnum::QUOTE, dxfcpp::EventTypeEnum::CANDLE});
//        endpoint->connect("demo.dxfeed.com:7300");
//
//        std::this_thread::sleep_for(std::chrono::seconds(5));
//    }

    //std::cin.get();

    //    using namespace dxfcpp;
    //
    //    auto endpoint = DXEndpoint::newBuilder()
    //            ->withProperty("dxfeed.address", "demo.dxfeed.com:7300")
    //            ->build();
    //    auto subscription = endpoint->getFeed()->createSubscription(Quote::type);
    //    subscription->addEventListener([](auto&& events) {
    //        for (auto&& e : events) {
    //            std::cout << e << "\n";
    //        }
    //    });
    //    subscription->addSymbols({"AAPL"});
}
