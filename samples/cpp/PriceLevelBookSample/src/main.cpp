// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <mutex>
#include <string>

#include <fmt/format.h>

#include "PriceLevel.hpp"
#include "PriceLevelBook.hpp"

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

int main(int /*argc*/, char * /*argv*/[]) {
    try {
        const auto address = "demo.dxfeed.com:7300";

        // Disable QD logging.
        // Logging::init();

        auto symbol = "MSFT";
        auto source = OrderSource::NTV;

        auto book =
            PriceLevelBook<Order>::newBuilder()
                ->withFeed(DXFeed::getInstance())
                ->withSymbol(symbol)
                ->withSource(source)
                ->withDepthLimit(10)
                ->withAggregationPeriod(10s)
                ->withListener([](const std::vector<std::shared_ptr<PriceLevel>> &buyLevels,
                                  const std::vector<std::shared_ptr<PriceLevel>> &sellLevels) {
                    if (buyLevels.empty() && sellLevels.empty()) {
                        return;
                    }

                    std::cout << fmt::format("{:=^66}\n", "");
                    std::cout << fmt::format("{:^31} || {:^31}\n", "ASK", "BID");
                    std::cout << fmt::format("{0:^15}|{1:^15} || {0:^15}|{1:^15}\n", "Price", "Size");
                    std::cout << fmt::format("{:-^66}\n", "");

                    for (auto buyIt = buyLevels.begin(), sellIt = sellLevels.begin();
                         buyIt != buyLevels.end() || sellIt != sellLevels.end();) {
                        std::string row{};
                        if (buyIt != buyLevels.end()) {
                            row += fmt::format("{:>14.4f} | {:<14.2f}", (*buyIt)->getPrice(), (*buyIt)->getSize());

                            ++buyIt;
                        } else {
                            row += fmt::format("{:>14} | {:<14}", "", "");
                        }

                        row += " || ";

                        if (sellIt != sellLevels.end()) {
                            row += fmt::format("{:>14.4f} | {:<14.2f}", (*sellIt)->getPrice(), (*sellIt)->getSize());

                            ++sellIt;
                        } else {
                            row += fmt::format("{:>14} | {:<14}", "", "");
                        }

                        std::cout << row << std::endl;
                    }

                    std::cout << fmt::format("{:=^66}\n", "");
                })
                ->build();

        DXEndpoint::getInstance()->connect(address);

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}