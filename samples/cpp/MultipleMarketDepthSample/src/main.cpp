// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <mutex>
#include <random>
#include <string>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include "MultipleMarketDepthModel.hpp"

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

// This sample program demonstrates how to use the MultipleMarketDepthModel
int main(int /*argc*/, char * /*argv*/[]) {
    try {
        const auto address = "demo.dxfeed.com:7300";

        // Disable QD logging.
        // Logging::init();

        std::recursive_mutex ioMutex{};
        auto symbol = IndexedEventSubscriptionSymbol("AAPL", OrderSource::NTV);
        auto printBook = [&ioMutex](const IndexedEventSubscriptionSymbol &symbol,
                                    const std::shared_ptr<OrderBook<Order>> &book) {
            std::lock_guard lock{ioMutex};

            std::string result{};

            result += fmt::format("{}:\n", symbol.toString());

            const auto maxCount = std::max(book->buy.size(), book->sell.size());

            for (std::size_t i = 0; i < maxCount; i++) {
                auto buyTable = i < book->buy.size() ? fmt::format("Buy  [Source: {}, Size: {:8.2f}, Price: {:8.4f}]",
                                                                   book->buy[i]->getSource().toString(),
                                                                   book->buy[i]->getSize(), book->buy[i]->getPrice())
                                                     : "Buy  [None]"s;
                auto sellTable = i < book->sell.size()
                                     ? fmt::format("Sell  [Source: {}, Size: {:8.2f}, Price: {:8.4f}]",
                                                   book->sell[i]->getSource().toString(), book->sell[i]->getSize(),
                                                   book->sell[i]->getPrice())
                                     : "Sell  [None]"s;
                result += fmt::format("{} \t {}\n", buyTable, sellTable);
            }

            std::cout << result << std::endl;
        };

        auto model = MultipleMarketDepthModel<Order>::newBuilder()
                         ->withFeed(DXFeed::getInstance())
                         ->withDepthLimit(10)
                         ->withAggregationPeriod(10s)
                         ->withListener(printBook)
                         ->build();
        model->addSymbol(symbol);
        model->addSymbol(IndexedEventSubscriptionSymbol("TSLA", OrderSource::NTV));

        DXEndpoint::getInstance()->connect(address);

        std::cout << fmt::format("Press [ENTER] to print {} order book manually.", symbol.toString()) << std::endl;

        while (true) {
            std::cin.ignore();

            auto result = model->tryGetBook(symbol);

            if (result.first) {
                std::lock_guard lock{ioMutex};
                std::cout << "=============================Print manually=============================" << std::endl;
                printBook(symbol, result.second);
                std::cout << "========================================================================" << std::endl;
            }
        }
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}