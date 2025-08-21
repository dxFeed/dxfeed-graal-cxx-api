// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

void printUsage() {
    const auto usageString = R"(
Usage:
CandleDataSample <address> <user> <password> <baseSymbol> <fromTime> <toTime>

Where:
    address    - The address to connect to retrieve data.
                 Examples:
                     https://tools-demo.dxfeed.com/candledata-preview
                     https://tools.dxfeed.com/candledata
    user       - User login name.
    password   - User password.
    baseSymbol - Base symbol name for Candle or TnS to get events for (e.g. "IBM", "AAPL", "MSFT").
    fromTime   - Is from-time for history subscription in standard formats.
                 Same examples of valid from-time:
                     20070101-123456
                     20070101-123456.123
                     2005-12-31 21:00:00
                     2005-12-31 21:00:00.123+03:00
                     2005-12-31 21:00:00.123+0400
                     2007-11-02Z
                     123456789 - value-in-milliseconds
    toTime     - Is to-time for history subscription in standard formats.

Examples:
    CandleDataSample https://tools-demo.dxfeed.com/candledata-preview demo demo IBM 20250818-120000 20250819-130000)";

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 7) {
            printUsage();

            return 0;
        }

        // Disable QD logging.
        Logging::init();

        // Parse args.
        const auto address = argv[1];
        const auto user = argv[2];
        const auto password = argv[3];
        const auto baseSymbol = argv[4];
        const auto fromTime = TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse(argv[5]);
        const auto toTime = TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse(argv[6]);

        const auto builder =
            HistoryEndpoint::newBuilder()->withAddress(address)->withUserName(user)->withPassword(password);
        const auto endpoint = builder->build();
        const auto res = endpoint->getTimeSeries<Candle>(
            CandleSymbol::valueOf(baseSymbol, CandlePeriod::valueOf(1, CandleType::HOUR)) /*{=h}*/, fromTime, toTime);

        std::cout << "Received candle count: " << res.size() << std::endl;
        const auto resTns = builder->build()->getTimeSeries<TimeAndSale>(baseSymbol, fromTime, toTime);

        std::cout << "Received tns count: " << resTns.size() << std::endl;
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}