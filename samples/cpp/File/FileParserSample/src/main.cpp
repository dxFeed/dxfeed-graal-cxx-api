// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <mutex>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

void printUsage() {
    const auto usageString = R"(
Usage:
FileParserSample <file> <type> <symbol>

Where:
    file    - Is a file name.
    types   - Is comma-separated list of dxfeed event types ()" +
                       enum_utils::getEventTypeEnumNamesList() + " or " +
                       enum_utils::getEventTypeEnumClassNamesList() + R"().
    symbols - Is comma-separated list of symbol names to get events for (e.g. "IBM,AAPL,MSFT").)";

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 4) {
            printUsage();

            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        std::atomic<std::size_t> eventCounter{};
        std::mutex ioMtx{};

        // Parse args.
        const std::string fileName = argv[1];
        auto [parsedTypes, unknownTypes] = CmdArgsUtils::parseTypes(argv[2]);
        const auto symbols = CmdArgsUtils::parseSymbols(argv[3]);

        // Create endpoint specifically for file parsing.
        const auto endpoint = DXEndpoint::create(DXEndpoint::Role::STREAM_FEED);
        const auto feed = endpoint->getFeed();

        // Subscribe to a specified event and symbol.
        const auto sub = feed->createSubscription(parsedTypes);
        sub->addEventListener([&eventCounter, &ioMtx](const auto &events) {
            std::lock_guard lock{ioMtx};

            for (auto &&e : events) {
                std::cout << ++eventCounter << ": " << e << "\n";
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
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}