// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>

void fetchAndPrint(const dxfcpp::CandleSymbol &candleSymbol, std::int64_t toTime, std::int64_t fromTime) {
    // Use the default DXFeed instance for that data feed address is defined by "dxfeed.system.properties" file
    const auto result = dxfcpp::DXFeed::getInstance()
                            ->getTimeSeriesPromise<dxfcpp::Candle>(candleSymbol, fromTime, toTime)
                            ->await(std::chrono::seconds(5));

    for (const auto &candle : result) {
        std::cout << candle->toString() << "\n";
    }
}

// Fetches last 20 days of candles for a specified symbol, prints them, and exits.
int main(int argc, char *argv[]) {
    static constexpr auto DAYS = 20;

    using namespace dxfcpp;
    using namespace std::string_literals;

    try {
        if (argc < 2) {
            std::cout << R"(
Usage:
FetchDailyCandlesSample <baseSymbol>

Where:
    baseSymbol - Is security symbol (e.g. IBM, AAPL, SPX etc.)
)";
            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        // Specified instrument name, for example AAPL, IBM, MSFT, etc.
        const auto baseSymbol = argv[1];
        const auto candleSymbol = CandleSymbol::valueOf(baseSymbol, CandlePeriod::DAY);
        const auto toTime = now();
        const auto fromTime = (std::chrono::milliseconds(toTime) - std::chrono::days(DAYS)).count();

        std::cout << "Fetching last " << DAYS << " days of candles for " << baseSymbol << "\n";

        fetchAndPrint(candleSymbol, toTime, fromTime);
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
