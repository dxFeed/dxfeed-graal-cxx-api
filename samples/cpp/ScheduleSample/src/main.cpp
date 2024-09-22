// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>

using namespace dxfcpp;
using namespace std::string_literals;

void printUsage() {
    std::cout << R"(usage:  ScheduleSample  <defaults>  <profiles>  <symbol>  [time]
where:  <defaults>  is a path to Schedule API defaults file
        <profiles>  is a URL to IPF file
        <symbol>    is a ticker symbol used for sample
        [time]      is a time used for sample in a format yyyy-MM-dd-HH:mm:ss
sample: ScheduleSample schedule.properties sample.ipf.zip IBM 2011-05-26-14:15:00)"
              << std::endl;
}

void updateScheduleDefaults(const std::string &url) {
    std::ifstream is(url);

    if (!is.is_open()) {
        std::cerr << "Failed to open " + url << std::endl;

        return;
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>{is}, {});

    Schedule::setDefaults(buffer);
}

std::unordered_map<std::string, std::shared_ptr<InstrumentProfile>> loadInstrumentProfiles(const std::string &url) {
    std::unordered_map<std::string, std::shared_ptr<InstrumentProfile>> profiles{};

    for (auto &&profile : InstrumentProfileReader::create()->readFromFile(url)) {
        profiles[profile->getSymbol()] = profile;
    }

    std::cout << "Loaded " << profiles.size() << " instrument profiles" << std::endl;

    return profiles;
}

void checkAllSchedules(auto &&profiles) {
    auto successes = 0;

    for (auto &&[symbol, profile] : profiles) {
        bool error = false;

        if (!Schedule::getInstance(profile)) {
            error = true;
        } else {
            for (auto &&venue : Schedule::getTradingVenues(profile)) {
                if (!Schedule::getInstance(profile, venue)) {
                    error = true;
                    break;
                }
            }
        }

        if (error) {
            std::cerr << "Error getting schedule for " + profile->getSymbol() + " (" + profile->getTradingHours() +
                             "): "
                      << std::endl;
        } else {
            successes++;
        }
    }

    std::cout << "Checked " << profiles.size() << " instrument profiles: " << successes << " successes, "
              << (profiles.size() - successes) << " failures" << std::endl;
}

void printNext5Holidays(auto &&profile, auto time) {
    auto schedule = Schedule::getInstance(profile);
    auto day = schedule->getDayByTime(time);

    std::string output = "5 next holidays for " + profile->getSymbol() + ":";

    if (!day) {
        output += "Could not get day by time";
    } else {
        for (auto i = 0; i < 5; i++) {
            day = day->findNextDay(DayFilter::HOLIDAY);

            if (!day) {
                break;
            }

            output += " " + std::to_string(day->getYearMonthDay());
        }
    }

    std::cout << output << std::endl;
}

void printCurrentSession(auto &&profile, auto time) {
    auto schedule = Schedule::getInstance(profile);

    if (!schedule) {
        std::cerr << "Error getting schedule for " + profile->getSymbol() + " (" + profile->getTradingHours() + "): "
                  << std::endl;

        return;
    }

    auto session = schedule->getSessionByTime(time);

    if (!session) {
        std::cerr << "Error getting session for time: " + TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(time)
                  << std::endl;

        return;
    }

    std::cout << "Current session for " + profile->getSymbol() + ": " + session->toString() + " in " +
                     session->getDay()->toString()
              << std::endl;
}

void printNextTradingSession(auto &&profile, auto time) {
    auto schedule = Schedule::getInstance(profile);

    if (!schedule) {
        std::cerr << "Error getting schedule for " + profile->getSymbol() + " (" + profile->getTradingHours() + "): "
                  << std::endl;

        return;
    }

    auto session = schedule->getSessionByTime(time);

    if (!session) {
        std::cerr << "Error getting session for time: " + TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(time)
                  << std::endl;

        return;
    }

    if (!session->isTrading()) {
        session = session->getNextSession(SessionFilter::TRADING);
    }

    if (!session) {
        std::cerr << "There is no next trading session for " << profile->getSymbol() << std::endl;
    } else {
        std::cout << "Next trading session for " + profile->getSymbol() + ": " + session->toString() + " in " +
                         session->getDay()->toString()
                  << std::endl;
    }
}

void printNearestTradingSession(auto &&profile, auto time) {
    auto schedule = Schedule::getInstance(profile);

    if (!schedule) {
        std::cerr << "Error getting schedule for " + profile->getSymbol() + " (" + profile->getTradingHours() + "): "
                  << std::endl;

        return;
    }

    auto session = schedule->getNearestSessionByTime(time, SessionFilter::TRADING);

    if (!session) {
        std::cerr << "There is no nearest trading session for " << profile->getSymbol() << std::endl;
    } else {
        std::cout << "Nearest trading session for " + profile->getSymbol() + ": " + session->toString() + " in " +
                         session->getDay()->toString()
                  << std::endl;
    }
}

/**
 * A sample program that demonstrates different use cases of Schedule API.
 */
int main(int argc, char *argv[]) {
    try {
        if (argc > 1 && (argv[1] == "-h"s || argv[1] == "--help"s)) {
            printUsage();

            return 0;
        }

        if (argc < 4 || argc > 5) {
            std::cerr << "Wrong number of arguments" << std::endl;
            printUsage();

            return 1;
        }

        updateScheduleDefaults(argv[1]);

        auto profiles = loadInstrumentProfiles(argv[2]);

        checkAllSchedules(profiles);

        std::string symbol = argv[3];

        if (!profiles.contains(symbol)) {
            std::cerr << "Could not find profile for " + symbol << std::endl;

            return 2;
        }

        auto profile = profiles[symbol];

        std::cout << "Found profile for " + symbol + ": " + profile->getDescription() << std::endl;

        auto time = argc < 5 ? dxfcpp::now() : TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse(argv[4]);

        std::cout << "Using timestamp " + TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(time) << std::endl;

        printNext5Holidays(profile, time);
        printCurrentSession(profile, time);
        printNextTradingSession(profile, time);
        printNearestTradingSession(profile, time);
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';
    }
}