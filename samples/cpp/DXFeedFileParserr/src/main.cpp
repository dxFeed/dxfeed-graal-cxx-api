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
DxFeedFileParser <file> <type> <symbol>

Where:
    file   - Is a file name.
    type   - Is comma-separated list of dxfeed event types ()" +
                       dxfcpp::enum_utils::getEventTypeEnumNamesList() + R"().
    symbol - Is comma-separated list of symbol names to get events for (e.g. "IBM,AAPL,MSFT").)";

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printUsage();

        return 0;
    }

    std::atomic<std::size_t> eventCounter{};
    std::mutex ioMtx{};

    // Parse args.
    std::string fileName = argv[1];
    auto types = CmdArgsUtils::parseTypes(argv[2]);
    auto symbols = CmdArgsUtils::parseSymbols(argv[3]);

    // Create endpoint specifically for file parsing.
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::STREAM_FEED);
    auto feed = endpoint->getFeed();

    // Subscribe to a specified event and symbol.
    auto sub = feed->createSubscription(types);
    sub->addEventListener([&eventCounter, &ioMtx](const auto &events) {
        std::lock_guard lock{ioMtx};

        for (auto &&e : events) {
            std::cout << (++eventCounter) << ": " << e << "\n";
        }
    });

    // Add symbols.
    sub->addSymbols(symbols);

    // Connect endpoint to a file.
    endpoint->connect("file:" + fileName + "[speed=max]");

    // Wait until file is completely parsed.
    endpoint->awaitNotConnected();

    // Close endpoint when we're done.
    // This method will gracefully close endpoint, waiting while data processing completes.
    endpoint->closeAndAwaitTermination();

    return 0;
}