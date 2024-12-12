// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

int main() {
    try {
        const auto address = "demo.dxfeed.com:7300";
        auto symbol = "AAPL";
        auto recordPrintLimit = 10u;

        // Disable QD logging.
        // Logging::init();

        auto eventPrinter = [symbol, recordPrintLimit](const IndexedEventSource &source,
                                                       const std::vector<std::shared_ptr<Order>> &events,
                                                       bool isSnapshot) {
            std::cout << fmt::format("Snapshot {{ Symbol: '{}', Source: '{}', Type: {} }}", symbol, source.toString(),
                                     isSnapshot ? "full" : "update")
                      << std::endl;

            std::size_t count{};

            for (const auto &order : events) {
                if (!EventFlag::isRemove(order) && order->hasSize()) {
                    std::cout << "    " << order->toString() << std::endl;
                } else {
                    std::cout << fmt::format("    REMOVAL {{ Index: {:#x} }}", order->getIndex()) << std::endl;
                }

                if (++count < recordPrintLimit || recordPrintLimit == 0) {
                    continue;
                }

                if (events.size() - count > 0) {
                    std::cout << fmt::format("   {{ ... {} records left ...}}", events.size() - count) << std::endl;
                }

                break;
            }
        };

        // Build the model with the specified configurations.
        auto model = IndexedTxModel<Order>::newBuilder()
                         ->withBatchProcessing(true)       // Enable batch processing.
                         ->withSnapshotProcessing(true)    // Enable snapshot processing.
                         ->withFeed(DXFeed::getInstance()) // Attach the model to the DXFeed instance.
                         ->withSymbol(symbol)              // Set the symbol to subscribe to.
                         ->withSources({OrderSource::NTV}) // Set the source for indexed events.
                         ->withListener([&eventPrinter](const auto &source, const auto &events, bool isSnapshot) {
                             eventPrinter(source, events, isSnapshot);
                         })
                         ->build();

        DXEndpoint::getInstance()->connect(address);

        std::cout << fmt::format("Subscribed to market depth updates for {}. Press [ENTER] to exit.", symbol)
                  << std::endl;

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}