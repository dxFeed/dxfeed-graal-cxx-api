// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

static const std::string DXFEED_IPF_URL = "https://demo:demo@tools.dxfeed.com/ipf";

void printUsage() {
    const auto usageString = R"(
Usage:
IpfLiveSample [<ipf-url>] [--duration <seconds>]

Where:
    <ipf-url>  - URL for the instruments profiles, default: )" +
                             DXFEED_IPF_URL + R"(
    --duration - Run for the given number of seconds and exit: the exit code is 0 if a profile was loaded,
                 1 otherwise. Without it the sample runs until it is stopped.)";

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

        // Disable QD logging.
        // Logging::init();

        std::string url = DXFEED_IPF_URL;
        std::optional<std::chrono::duration<double>> duration{};

        for (int i = 1; i < argc; i++) {
            if (argv[i] == "--duration"s) {
                if (i + 1 == argc) {
                    std::cerr << "No value of --duration\n";
                    printUsage();

                    return 1;
                }

                duration = std::chrono::duration<double>(std::stod(argv[++i]));
            } else if (i == 1) {
                url = argv[i];
            } else {
                std::cerr << "Unexpected argument: " << argv[i] << '\n';
                printUsage();

                return 1;
            }
        }

        auto collector = InstrumentProfileCollector::create();
        const auto connection = InstrumentProfileConnection::createConnection(url, collector);

        // Update period can be used to re-read IPF files, not needed for services supporting IPF "live-update"
        connection->setUpdatePeriod(std::chrono::seconds(60));

        connection->addStateChangeListener([](auto /* InstrumentProfileConnection::State oldState */, auto newState) {
            std::cout << "Connection state: " + InstrumentProfileConnection::stateToString(newState) << std::endl;
        });

        connection->start();

        // We can wait until we get the first full snapshot of instrument profiles
        connection->waitUntilCompleted(std::chrono::seconds(10));

        // Data model to keep all instrument profiles mapped by their ticker symbol
        std::unordered_map<std::string, std::shared_ptr<InstrumentProfile>> profiles{};
        std::mutex mutex{};

        // It is possible to add listener after connection is started - updates will not be missed in this case
        // The listener does not own the collector: the collector owns the listener
        collector->addUpdateListener([&profiles, &mutex, weakCollector = std::weak_ptr(collector)](auto &&instruments) {
            const auto self = weakCollector.lock();

            if (!self) {
                return;
            }

            std::cout << "\nInstrument Profiles:" << std::endl;
            // We can observe REMOVED elements - need to add necessary filtering
            // See documentation for `InstrumentProfileCollector` for more details

            // (1) We can either process instrument profile updates manually
            for (auto &&profile : instruments) {
                std::lock_guard lock{mutex};

                if (InstrumentProfileType::REMOVED.getName() == profile->getType()) {
                    // Profile was removed - remove it from our data model
                    profiles.erase(profile->getSymbol());
                } else {
                    // Profile was updated - collector only notifies us if the profile was changed
                    profiles[profile->getSymbol()] = profile;
                }
            }

            {
                std::lock_guard lock{mutex};
                std::cout << "Total number of profiles (1): " + std::to_string(profiles.size()) << std::endl;
            }

            // (2) or access the concurrent view of instrument profiles
            std::unordered_set<std::string> symbols{};
            const auto view = self->view();

            while (view->hasNext()) {
                if (const auto ip = view->next(); ip->getType() != InstrumentProfileType::REMOVED.getName()) {
                    symbols.emplace(ip->getSymbol());
                }
            }

            std::cout << "Total number of profiles (2): " + std::to_string(symbols.size()) << std::endl;

            std::cout << "Last modified: " +
                             TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(self->getLastUpdateTime())
                      << std::endl;
        });

        if (duration) {
            std::this_thread::sleep_for(*duration);
            connection->close();

            std::size_t profileCount = 0;
            const auto view = collector->view();

            while (view->hasNext()) {
                if (view->next()->getType() != InstrumentProfileType::REMOVED.getName()) {
                    profileCount++;
                }
            }

            std::cout << "Loaded " << profileCount << " instrument profiles" << std::endl;

            return profileCount > 0 ? 0 : 1;
        }

        std::this_thread::sleep_for(std::chrono::days(365));

        connection->close();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    } catch (const std::exception &e) {
        // For example, std::stod of an invalid number.
        std::cerr << e.what() << '\n';

        return 1;
    }

    return 0;
}