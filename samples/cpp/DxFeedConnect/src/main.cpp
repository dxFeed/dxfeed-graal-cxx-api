// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

#include <range/v3/all.hpp>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

void printUsage() {
    auto usageString = R"(
Usage:
DxFeedConnect <address> <types> <symbols> [<time>]

Where:
    address - The address to connect to retrieve data (remote host or local tape file).
              To pass an authorization token, add to the address: "[login=entitle:<token>]",
              e.g.: demo.dxfeed.com:7300[login=entitle:<token>]
    types   - Is comma-separated list of dxfeed event types ()" +
                       dxfcpp::enum_utils::getEventTypeEnumNamesList() + " or " +
                       dxfcpp::enum_utils::getEventTypeEnumClassNamesList() + R"().
    symbols - Is comma-separated list of symbol names to get events for (e.g. "IBM,AAPL,MSFT").
              for Candle event specify symbol with aggregation like in "AAPL{=d}"
    time    - Is from-time for history subscription in standard formats.
              Same examples of valid from-time:
                  20070101-123456
                  20070101-123456.123
                  2005-12-31 21:00:00
                  2005-12-31 21:00:00.123+03:00
                  2005-12-31 21:00:00.123+0400
                  2007-11-02Z
                  123456789 - value-in-milliseconds

Examples:
    DxFeedConnect demo.dxfeed.com:7300 Quote,Trade MSFT,IBM
    DxFeedConnect demo.dxfeed.com:7300 TimeAndSale AAPL
    DxFeedConnect demo.dxfeed.com:7300 Candle AAPL{=d} 20230901Z)";

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printUsage();

        return 0;
    }

    std::mutex ioMtx{};

    // Parse args.
    std::string address = argv[1];
    auto types = CmdArgsUtils::parseTypes(argv[2]);
    auto symbols = CmdArgsUtils::parseSymbols(argv[3]);
    auto time = -1LL;

    if (argc >= 5) {
        time = TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse(argv[4]);
    }

    // Create an endpoint and connect to specified address.
    auto endpoint = DXEndpoint::create()->connect(address);

    // Create a subscription with specified types attached to feed.
    auto sub = endpoint->getFeed()->createSubscription(types);

    // Add an event listener.
    sub->addEventListener([&ioMtx](const auto &events) {
        std::lock_guard lock{ioMtx};

        for (auto &&e : events) {
            std::cout << e << "\n";
        }
    });

    // Add symbols.
    if (time != -1) {
        sub->addSymbols(symbols | ranges::views::transform([time](const auto &s) {
                            return TimeSeriesSubscriptionSymbol(s, time);
                        }));
    } else {
        sub->addSymbols(symbols);
    }

    std::cin.get();

    return 0;
}