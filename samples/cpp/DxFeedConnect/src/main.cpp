// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <mutex>

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
                       dxfcpp::enum_utils::getEventTypeEnumNamesList() + R"().
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
    DXFeedConnect demo.dxfeed.com:7300 Quote,Trade MSFT,IBM
    DXFeedConnect demo.dxfeed.com:7300 TimeAndSale AAPL
    DXFeedConnect demo.dxfeed.com:7300 Candle AAPL{=d} 20230201Z)";

    std::cout << usageString << std::endl;
}

struct E {
    using KeyType = std::int64_t;
    using ValueType = std::string;

    static const E STUB;

    KeyType key{};
    ValueType value{};

    const KeyType &getKey() const noexcept {
        return key;
    }

    const ValueType &getValue() const noexcept {
        return value;
    }
};

const E E::STUB{};

int main(int argc, char *argv[]) {
    TimeFormat::Cache<E> cache{};

    cache.add({1, "1"});

    std::cout << "1:\n";

    if (auto valueRefOpt = cache.getEntryValue(1); valueRefOpt) {
        std::cout << valueRefOpt->get() << std::endl;
    }

    std::cout << "257:\n";

    cache.add({257, "257"});

    if (auto valueRefOpt = cache.getEntryValue(1); valueRefOpt) {
        std::cout << valueRefOpt->get() << std::endl;
    } else {
        std::cout << " -- 1 " << std::endl;
    }

    if (auto valueRefOpt = cache.getEntryValue(257); valueRefOpt) {
        std::cout << valueRefOpt->get() << std::endl;
    } else {
        std::cout << " -- 257 " << std::endl;
    }

    return 0;

    if (argc < 4) {
        printUsage();

        return 0;
    }

    std::atomic<std::size_t> eventCounter{};
    std::mutex ioMtx{};

    // Parse args.
    std::string address = argv[1];
    auto types = CmdArgsUtils::parseTypes(argv[2]);
    auto symbols = CmdArgsUtils::parseSymbols(argv[3]);

    if (argc >= 5) {
        // std::chrono::parse
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
    sub->addSymbols(symbols);

    std::cin.get();

    return 0;
}