// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_c_api/api.h>
#include <thread>

using namespace dxfcpp;
using namespace std::chrono_literals;

std::atomic<bool> isThreadWork_{true};
std::atomic<int> eventCounter_{0};
std::atomic<int> listenerCounter_{0};

int main(int argc, char *argv[]) {
    std::thread t([&](){
        auto period = 2s;
        while(isThreadWork_.load()) {
            std::this_thread::sleep_for(period);
            double eventsPerSec = eventCounter_.exchange(0) / static_cast<double>(period.count());
            double listenerCallsPerSec = listenerCounter_.exchange(0) / static_cast<double>(period.count());

            std::cout << "\n" << "----------------------------------------------" << std::endl;
            std::cout << "Events                   : " << std::fixed << std::setprecision(2) << eventsPerSec << std::endl;
            std::cout << "Listener Calls           : " << listenerCallsPerSec << std::endl;
            std::cout << "Average Number of Events : " << eventsPerSec / listenerCallsPerSec << std::endl;
        }
    });

    auto endpoint = DXEndpoint::newBuilder()
            ->withProperty("dxfeed.address", argv[1])
            ->build();
    
    auto subscription = endpoint->getFeed()->createSubscription(TimeAndSale::Type);
    
    subscription->addEventListener([](std::vector<std::shared_ptr<EventType>> events) {
        eventCounter_ += events.size();
        ++listenerCounter_;
    });
    
    subscription->addSymbol(std::string(argv[2]));

    std::cin.get();

    isThreadWork_.exchange(false);
    t.join();
};
