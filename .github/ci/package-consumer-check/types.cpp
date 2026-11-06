// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The types of the API in a program built with hidden visibility: an exception that the library throws is caught by its
// type, and dynamic_pointer_cast finds the class of an event. Both need the type information exported by the shared
// library. Exit code 0 when both work.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>
#include <memory>

int main() {
    using namespace dxfcpp;
    using namespace std::chrono_literals;

    const std::shared_ptr<EventType> event = std::make_shared<Quote>("AAPL");

    if (!std::dynamic_pointer_cast<Quote>(event) || std::dynamic_pointer_cast<Trade>(event)) {
        std::cerr << "FAIL: dynamic_pointer_cast does not find the class of the event\n";

        return 1;
    }

    try {
        const auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);

        try {
            // A negative aggregation period is rejected by Java.
            endpoint->setRequestedAggregationPeriod(-5ms);
            std::cerr << "FAIL: no exception\n";

            return 1;
        } catch (const JavaException &e) {
            std::cout << "OK: caught JavaException: " << e.what() << '\n';
        }

        endpoint->closeAndAwaitTermination();
    } catch (const std::exception &e) {
        std::cerr << "FAIL: JavaException is not caught by its type: " << e.what() << '\n';

        return 1;
    }

    std::cout << "OK: dynamic_pointer_cast<Quote>" << std::endl;

    return 0;
}
