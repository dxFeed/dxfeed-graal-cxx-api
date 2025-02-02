// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

void printUsage() {
    auto usageString = R"(
Usage:
DxFeedIpfConnect <type> <ipf-file>

Where:
    type     - Is dxfeed event type ()" +
                       dxfcpp::enum_utils::getMarketEventTypeEnumNamesList() + " or " +
                       dxfcpp::enum_utils::getMarketEventTypeEnumClassNamesList() + R"().
    ipf-file - Is name of instrument profiles file.

Examples:
    DxFeedIpfConnect Quote https://dxfeed.s3.amazonaws.com/masterdata/ipf/demo/mux-demo.ipf.zip
)";

    std::cout << usageString << std::endl;
}

std::vector<std::string> getSymbols(const std::string &filename) noexcept {
    std::cout << "Reading instruments from " + filename + "...\n";

    auto profiles = dxfcpp::InstrumentProfileReader::create()->readFromFile(filename);

    auto profileFilter = [](auto &&profile) -> bool {
        // This is just a sample, any arbitrary filtering may go here.
        return profile->getType() == "STOCK" &&                           // stocks
               profile->getSIC() / 10 == 357 &&                           // Computer And Office Equipment
               profile->getExchanges().find("XNYS") != std::string::npos; // traded at NYSE
    };

    std::vector<std::string> result{};

    std::cout << "Selected symbols are:\n";

    for (auto &&profile : profiles) {
        if (profileFilter(profile)) {
            result.push_back(profile->getSymbol());
            std::cout << profile->getSymbol() + " (" + profile->getDescription() + ")\n";
        }
    }

    return result;
}

/*
 * This sample demonstrates how to subscribe to available symbols using IPF.
 * Use default DXFeed instance for that data feed address is defined by "dxfeed.properties" file.
 */
int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;

    try {
        if (argc < 3) {
            printUsage();

            return 0;
        }

        auto types = CmdArgsUtils::parseTypes(argv[1]);
        auto ipfFile = argv[2];

        auto sub = DXFeed::getInstance()->createSubscription(types.first);

        sub->addEventListener<MarketEvent>([](auto &&events) {
            for (auto &&event : events) {
                std::cout << event->getEventSymbol() + ": " + event->toString() << std::endl;
            }
        });

        sub->addSymbols(getSymbols(ipfFile));

        std::this_thread::sleep_for(std::chrono::days(365));
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
