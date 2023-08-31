// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <range/v3/view.hpp>
#include <range/v3/range.hpp>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

void printUsage() {
    auto evenTypeEnums = EventTypeEnum::ALL;

    auto x = ranges::join_with_view(evenTypeEnums | ranges::views::transform([](const auto& enumElement) {
                                        return enumElement.get().getName();
                                    }), ",");

    for (auto c : x) {
        std::cout << c << "{" << static_cast<std::size_t>(static_cast<unsigned char>(c)) << "}";
    }



//    var eventTypeNames = ReflectionUtil.CreateTypesString(IEventType.GetEventTypes());
//    var usageString = $@"
//                      Usage:
//        DxFeedFileParser <file> <type> <symbol>
//
//            Where:
//        file   - Is a file name.
//        type   - Is comma-separated list of dxfeed event types ({eventTypeNames}).
//                   symbol - Is comma-separated list of symbol names to get events for (e.g. ""IBM,AAPL,MSFT"").";
//                   Console.WriteLine(usageString);
}

/**
 * Converts one tape file into another tape file with optional intermediate processing or filtering.
 * This project contains sample tape file ConvertTapeFile.in.
 */
int main(int argc, char *argv[]) {
    printUsage();

    return 0;

    // Determine input and output tapes and specify appropriate configuration parameters.
    std::string inputAddress = argc > 1 ? argv[0] : "file:ConvertTapeFile.in[readAs=stream_data,speed=max]";
    std::string outputAddress = argc > 2 ? argv[1] : "tape:ConvertTapeFile.out[saveAs=stream_data,format=text]";

    // Create input endpoint configured for tape reading.
    auto inputEndpoint =
        DXEndpoint::newBuilder()
            ->withRole(DXEndpoint::Role::STREAM_FEED) // Prevents event conflation and loss due to buffer overflow.
            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enables wildcard subscription.
            ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")  // Use provided event times.
            ->build();

    // Create output endpoint configured for tape writing.
    auto outputEndpoint =
        DXEndpoint::newBuilder()
            ->withRole(DXEndpoint::Role::STREAM_PUBLISHER) // Prevents event conflation and loss due to buffer overflow.
            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enables wildcard subscription.
            ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")  // Use provided event times.
            ->build();

    // Create and link event processor for all types of events.
    // Note: Set of processed event types could be limited if needed.
    auto sub = inputEndpoint->getFeed()->createSubscription(EventTypeEnum::ALL);

    sub->addEventListener([outputEndpoint](auto events) {
        // Here event processing occurs. Events could be modified, removed, or new events added.
        // For example, the below code adds 1 hour to event times:
        //        for (auto &e : events) {
        //            if (auto event = e->template sharedAs<EventType>(); event) {
        //                event->setEventTime(event->getEventTime() + 3600'000);
        //            }
        //        }

        // Publish processed events
        auto publisher = outputEndpoint->getPublisher();

        publisher->publishEvents(events);
    });

    // Subscribe to all symbols.
    // Note: Set of processed symbols could be limited if needed.
    sub->addSymbols(WildcardSymbol::ALL);

    // Connect output endpoint and start output tape writing BEFORE starting input tape reading.
    outputEndpoint->connect(outputAddress);
    // Connect input endpoint and start input tape reading AFTER starting output tape writing.
    inputEndpoint->connect(inputAddress);

    // Wait until all data is read and processed, and then gracefully close input endpoint.
    inputEndpoint->awaitNotConnected();
    inputEndpoint->closeAndAwaitTermination();

    // Wait until all data is processed and written, and then gracefully close output endpoint.
    outputEndpoint->awaitProcessed();
    outputEndpoint->closeAndAwaitTermination();

    return 0;
}