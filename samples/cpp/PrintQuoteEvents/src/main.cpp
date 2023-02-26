// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxFeedNativeCppAPI.hpp>

int main() {
    dxfcpp::Quote q{};

    std::cout << dxfcpp::detail::nowStrWithTimeZone() << std::endl;
    std::cout << dxfcpp::detail::formatTimeStamp(1677448613000) << std::endl;
    std::cout << dxfcpp::detail::formatTimeStampWithMillis(1677448613023) << std::endl;

    {
        auto builder = dxfcpp::DXEndpoint::newBuilder()->withRole(dxfcpp::DXEndpoint::Role::FEED);
        auto endpoint = builder->build();

        endpoint->onStateChange() += [](dxfcpp::DXEndpoint::State oldState, dxfcpp::DXEndpoint::State newState) {
            dxfcpp::detail::debug("{}", std::string("State changed: ") + dxfcpp::DXEndpoint::stateToString(oldState) +
                                            " -> " + dxfcpp::DXEndpoint::stateToString(newState));
        };

        auto sub =
            endpoint->getFeed()->createSubscription({dxfcpp::EventTypeEnum::QUOTE, dxfcpp::EventTypeEnum::CANDLE});
        endpoint->connect("demo.dxfeed.com:7300");

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cin.get();

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
