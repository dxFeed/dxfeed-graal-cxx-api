// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

/// Write events to a tape file.
int main() {
    try {
        // Create an appropriate endpoint.
        auto endpoint = DXEndpoint::newBuilder()
                            // Is required for tape connector to be able to receive everything.
                            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                            ->withRole(dxfcpp::DXEndpoint::Role::PUBLISHER)
                            ->build();

        // Connect to the address, remove [format=text] or change on [format=binary] for binary format
        endpoint->connect("tape:WriteTapeFile.out.txt[format=text]");

        // Get publisher.
        auto pub = endpoint->getPublisher();

        // Creates new Quote market events.
        auto quote1 = std::make_shared<Quote>("TEST1")->withBidPrice(10.1).withAskPrice(10.2).sharedAs<Quote>();
        auto quote2 = std::make_shared<Quote>("TEST2")->withBidPrice(17.1).withAskPrice(17.2).sharedAs<Quote>();

        // Publish events.
        pub->publishEvents({quote1, quote2});

        // Wait until all data is written, close, and wait until it closes.
        endpoint->awaitProcessed();
        endpoint->closeAndAwaitTermination();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';
    }

    return 0;
}