// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

/**
 * Converts one tape file into another tape file with optional intermediate processing or filtering.
 * This project contains a sample tape file `ConvertTapeFile.in`.
 */
int main(int argc, char *argv[]) {
    try {
        // Disable QD logging.
        // Logging::init();

        // Determine input and output tapes and specify appropriate configuration parameters.
        const std::string inputAddress = argc > 1 ? argv[0] : "file:ConvertTapeFile.in[readAs=stream_data,speed=max]";
        const std::string outputAddress =
            argc > 2 ? argv[1] : "tape:ConvertTapeFile.out[saveAs=stream_data,format=text]";

        // Create an input endpoint configured for tape reading.
        const auto inputEndpoint =
            DXEndpoint::newBuilder()
                ->withRole(DXEndpoint::Role::STREAM_FEED) // Prevents event conflation and loss due to buffer overflow.
                ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enables wildcard subscription.
                ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")  // Use provided event times.
                ->build();

        // Create an output endpoint configured for tape writing.
        auto outputEndpoint =
            DXEndpoint::newBuilder()
                ->withRole(
                    DXEndpoint::Role::STREAM_PUBLISHER) // Prevents event conflation and loss due to buffer overflow.
                ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enables wildcard subscription.
                ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")  // Use provided event times.
                ->build();

        // Create and link event processor for all types of events.
        // Note: Set of processed event types could be limited if needed.
        const auto sub = inputEndpoint->getFeed()->createSubscription(EventTypeEnum::ALL);

        sub->addEventListener([outputEndpoint](auto events) {
            // Here event processing occurs. Events could be modified, removed, or new events added.
            // For example, the below code adds 1 hour to event times:
            //        for (auto &e : events) {
            //            if (auto event = e->template sharedAs<EventType>(); event) {
            //                event->setEventTime(event->getEventTime() + 3600'000);
            //            }
            //        }

            // Publish processed events
            const auto publisher = outputEndpoint->getPublisher();

            publisher->publishEvents(events);
        });

        // Subscribe to all symbols.
        // Note: Set of processed symbols could be limited if needed.
        sub->addSymbols(WildcardSymbol::ALL);

        // Connect output endpoint and start output tape writing BEFORE starting input tape reading.
        outputEndpoint->connect(outputAddress);
        // Connect input endpoint and start input tape reading AFTER starting output tape writing.
        inputEndpoint->connect(inputAddress);

        // Wait until all data is read and processed, and then gracefully close the input endpoint.
        inputEndpoint->awaitNotConnected();
        inputEndpoint->closeAndAwaitTermination();

        // Wait until all data is processed and written, and then gracefully close the output endpoint.
        outputEndpoint->awaitProcessed();
        outputEndpoint->closeAndAwaitTermination();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}