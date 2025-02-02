// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "fmt/format.h"
#include <dxfeed_graal_cpp_api/api.hpp>
#include <portals.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

namespace ot = org::ttldtor;
namespace otp = org::ttldtor::portals;

auto drawHeader() {
    std::cout << fmt::format("{:=^78}\n", "");
    std::cout << fmt::format("{:^38}||{:^38}\n", "ASK", "BID");
    std::cout << fmt::format("{0:^2}|{1:^12}|{2:^4}|{3:^8}|{4:^8}||{0:^2}|{1:^12}|{2:^4}|{3:^8}|{4:^8}\n", "EX",
                             "Source", "MM", "Price", "Size");
    std::cout << fmt::format("{:-^78}\n", "");
}

auto render(const std::vector<std::shared_ptr<Order>> &buy, const std::vector<std::shared_ptr<Order>> &sell,
            std::size_t depth) {
    for (std::size_t i = 0; i < depth; i++) {
        if (i < buy.size()) {
            std::cout << otp::moveTo(4 + i, 0) + fmt::format("{0:^2}|{1:^12}|{2:^4}|{3:^8.2}|{4:^8.2}",
                                                             buy[i]->getExchangeCodeString(),
                                                             buy[i]->getSource().toString(), buy[i]->getMarketMaker(),
                                                             buy[i]->getPrice(), buy[i]->getSize());
        }

        std::cout << otp::moveTo(4 + i, 38) + "||";

        if (i < sell.size()) {
            std::cout << otp::moveTo(4 + i, 40) +
                             fmt::format("{0:^2}|{1:^12}|{2:^4}|{3:^8.2}|{4:^8.2}", sell[i]->getExchangeCodeString(),
                                         sell[i]->getSource().toString(), sell[i]->getMarketMaker(),
                                         sell[i]->getPrice(), sell[i]->getSize());
        }
    }
}

int main(int argc, char *argv[]) {
    ot::VirtualTerminal::enable();
    otp::hideCursor();

    try {
        auto address = "demo.dxfeed.com:7300";
        auto symbol = "AAPL";
        auto sources = "NTV";
        // auto sources = "ntv";
        // auto sources = "AGGREGATE_ASK,AGGREDATE_BID";
        std::size_t depth = 10u;

        // Disable QD logging.
        Logging::init();
        std::mutex ioMtx{};

        auto endpoint = DXEndpoint::create();
        auto model = MarketDepthModel<Order>::newBuilder()
                         ->withFeed(endpoint->getFeed())
                         ->withListener([&depth, &ioMtx](const auto &buy, const auto &sell) {
                             std::lock_guard<std::mutex> lock(ioMtx);
                             render(buy, sell, depth);
                         })
                         ->withSources(CmdArgsUtils::parseEventSources(sources))
                         ->withSymbol(symbol)
                         ->withDepthLimit(depth)
                         ->withAggregationPeriod(30ms)
                         ->build();

        {
            std::lock_guard<std::mutex> lock(ioMtx);
            drawHeader();
        }

        endpoint->connect(address);

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    otp::showCursor();
    return 0;
}