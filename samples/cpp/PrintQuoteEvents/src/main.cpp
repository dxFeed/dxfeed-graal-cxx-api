// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_c_api/api.h>

using namespace dxfcpp;

int main() {
    auto endpoint = DXEndpoint::newBuilder()
            ->withProperty("dxfeed.address", "demo.dxfeed.com:7300")
            ->build();
    
    auto subscription = endpoint->getFeed()->createSubscription(Quote::Type);
    
    subscription->addEventListener([](std::vector<std::shared_ptr<EventType>> events) {
        for (auto&& e : events) {
            std::cout << std::static_pointer_cast<Quote>(e)->toString() << "\n";
        }
    });
    
    subscription->addSymbol(std::string("AAPL"));

    std::cin.get();
}
