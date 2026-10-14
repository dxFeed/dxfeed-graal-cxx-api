// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The check program of the -static-mt archives: publishes the quote AAPL (bid 10.5) to a LOCAL_HUB endpoint and
// receives it by a subscription. It needs no network, so it runs in a container without the Visual C++
// Redistributable. Exit code 0 when the quote is received in 10 seconds.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <vector>

#ifdef DXFCPP_USE_DLLS
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

int main() {
    using namespace dxfcpp;

    try {
        auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
        auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);
        auto received = std::make_shared<std::promise<std::string>>();
        auto result = received->get_future();

        subscription->addEventListener<Quote>([received](const auto &quotes) {
            for (const auto &quote : quotes) {
                if (quote->getBidPrice() == 10.5) {
                    try {
                        received->set_value(quote->toString());
                    } catch (const std::future_error &) {
                        // Already received.
                    }
                }
            }
        });
        subscription->addSymbols("AAPL");

        auto quote = std::make_shared<Quote>("AAPL");

        quote->setBidPrice(10.5);
        endpoint->getPublisher()->publishEvents(std::vector<std::shared_ptr<EventType>>{quote});

        if (result.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
            std::cerr << "FAIL: the quote AAPL is not received\n";

            return 1;
        }

        std::cout << "OK: " << result.get() << std::endl;
        endpoint->closeAndAwaitTermination();

#ifdef DXFCPP_USE_DLLS
        // Known issue: a process that uses the C++ API DLL hangs on exit (the isolate is torn down in
        // DLL_PROCESS_DETACH). TerminateProcess skips DLL_PROCESS_DETACH.
        TerminateProcess(GetCurrentProcess(), 0);
#endif

    } catch (const RuntimeException &e) {
        std::cerr << "FAIL: " << e << '\n';

        return 1;
    }

    return 0;
}
