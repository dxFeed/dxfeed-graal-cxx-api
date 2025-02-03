// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

static const std::string DXFEED_IPF_URL = "https://demo:demo@tools.dxfeed.com/ipf";

void printUsage() {
    auto usageString = R"(
Usage:
DxFeedLiveIpfSample [<ipf-url>]

Where:
    <ipf-url> - URL for the instruments profiles, default: )" +
                       DXFEED_IPF_URL;

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;

    try {
        if (argc > 1 && (argv[1] == "-h"s || argv[1] == "--help"s)) {
            printUsage();

            return 0;
        }

        std::string url = argc > 1 ? argv[1] : DXFEED_IPF_URL;

        auto collector = InstrumentProfileCollector::create();
        auto connection = InstrumentProfileConnection::createConnection(url, collector);
        // Update period can be used to re-read IPF files, not needed for services supporting IPF "live-update"
        connection->setUpdatePeriod(std::chrono::seconds(60));

        connection->addStateChangeListener([](auto /* InstrumentProfileConnection::State oldState */, auto newState) {
            std::cout << "Connection state: " + InstrumentProfileConnection::stateToString(newState) << std::endl;
        });

        connection->start();

        // We can wait until we get first full snapshot of instrument profiles
        connection->waitUntilCompleted(std::chrono::seconds(10));

        // Data model to keep all instrument profiles mapped by their ticker symbol
        std::unordered_map<std::string, std::shared_ptr<InstrumentProfile>> profiles{};
        std::mutex mutex{};

        // It is possible to add listener after connection is started - updates will not be missed in this case
        collector->addUpdateListener([&profiles, &mutex, self = collector](auto &&instruments) {
            std::cout << "\nInstrument Profiles:" << std::endl;
            // We can observe REMOVED elements - need to add necessary filtering
            // See javadoc for InstrumentProfileCollector for more details

            // (1) We can either process instrument profile updates manually
            for (auto &&profile : instruments) {
                std::lock_guard lock{mutex};

                if (InstrumentProfileType::REMOVED.getName() == profile->getType()) {
                    // Profile was removed - remove it from our data model
                    profiles.erase(profile->getSymbol());
                } else {
                    // Profile was updated - collector only notifies us if profile was changed
                    profiles[profile->getSymbol()] = profile;
                }
            }

            {
                std::lock_guard lock{mutex};
                std::cout << "Total number of profiles (1): " + std::to_string(profiles.size()) << std::endl;
            }

            // (2) or access the concurrent view of instrument profiles
            std::unordered_set<std::string> symbols{};
            auto view = self->view();

            while (view->hasNext()) {
                auto ip = view->next();

                if (ip->getType() != InstrumentProfileType::REMOVED.getName()) {
                    symbols.emplace(ip->getSymbol());
                }
            }

            std::cout << "Total number of profiles (2): " + std::to_string(symbols.size()) << std::endl;

            std::cout << "Last modified: " +
                             dxfcpp::TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(self->getLastUpdateTime())
                      << std::endl;
        });

        std::this_thread::sleep_for(std::chrono::days(365));

        connection->close();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}