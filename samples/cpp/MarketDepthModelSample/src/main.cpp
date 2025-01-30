// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <mutex>
#include <string>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

auto render(const auto& buy, const auto& sell) {

}

int main(int argc, char *argv[]) {
    try {
        auto address = "demo.dxfeed.com:7300";
        auto symbol = "AAPL";
        auto sources = "ntv";
        // auto sources = "AGGREGATE_ASK,AGGREDATE_BID";

        std::mutex ioMtx{};

        auto endpoint = DXEndpoint::create();
        auto model = MarketDepthModel<Order>::newBuilder()
            ->withFeed(endpoint->getFeed())
            ->withListener([](const auto& buy, const auto& sell) {
                                                                                render(buy, sell);
        })
                         ->withSources(CmdArgsUtils::parseEventSources(sources))
                         ->withSymbol(symbol)
                         ->withDepthLimit(10)
                         ->withAggregationPeriod(30ms)
                         ->build();

        endpoint->connect(address);

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}