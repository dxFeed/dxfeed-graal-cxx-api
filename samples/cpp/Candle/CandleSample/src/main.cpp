// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace dxfcpp;

void printUsage() {
    const auto usageString = R"(
Usage:
CandleSample <address> <symbol> [<fromTime>] [<attribute>=<value> ...]

Where:
    address   - The address to connect to retrieve data (e.g. "demo.dxfeed.com:7300") or a path to a tape file.
    symbol    - The candle symbol: a base symbol (e.g. "AAPL") with optional attributes (e.g. "AAPL{=d}").
    fromTime  - The from-time of the history subscription in standard formats (e.g. "2025-12-31",
                "20251231-123456", "1735689600000" in milliseconds). The default is 1970-01-01.
    attribute - Adds an attribute to the symbol:
                    exchange   - The exchange code letter (e.g. "exchange=A").
                    period     - The number of aggregation period units (e.g. "period=5"). The default is 1.
                    type       - The aggregation period type: "t" (TICK), "s" (SECOND), "m" (MINUTE),
                                 "h" (HOUR), "d" (DAY), "w" (WEEK), "mo" (MONTH), "o" (OPTEXP), "y" (YEAR),
                                 "v" (VOLUME), "p" (PRICE), "pm" (PRICE_MOMENTUM), "pr" (PRICE_RENKO).
                                 The default is TICK.
                    price      - The price type: "last", "bid", "ask", "mark", "s" (SETTLEMENT).
                    session    - "true" for the regular trading hours only, "false" for any session.
                    alignment  - The alignment: "m" (MIDNIGHT), "s" (SESSION).
                    priceLevel - The price level (e.g. "priceLevel=0.5").

Examples:
    CandleSample demo.dxfeed.com:7300 AAPL{=d} 2025-12-01
    CandleSample demo.dxfeed.com:7300 AAPL 2025-12-01 period=5 type=m session=true
)";

    std::cout << usageString << std::endl;
}

/*
 * Subscribes to the Candle events of a symbol from the specified time and prints them.
 */
int main(int argc, char *argv[]) {
    try {
        if (argc < 3) {
            printUsage();

            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        const std::string address = argv[1];
        const std::string baseSymbol = argv[2];
        std::int64_t fromTime = 0;
        std::vector<CandleSymbolAttributeVariant> attributes{};
        std::optional<double> periodValue{};
        std::optional<std::string> periodType{};

        for (int i = 3; i < argc; i++) {
            const std::string arg = argv[i];
            const auto separator = arg.find('=');

            if (separator == std::string::npos) {
                if (i != 3) {
                    std::cerr << "Unexpected argument: " << arg << '\n';
                    printUsage();

                    return 1;
                }

                fromTime = TimeFormat::DEFAULT.parse(arg);

                continue;
            }

            const auto name = arg.substr(0, separator);
            const auto value = arg.substr(separator + 1);

            if (name == "exchange" && value.size() == 1) {
                attributes.emplace_back(CandleExchange::valueOf(value[0]));
            } else if (name == "period") {
                periodValue = std::stod(value);
            } else if (name == "type") {
                periodType = value;
            } else if (name == "price") {
                attributes.emplace_back(CandlePrice::parse(value).get());
            } else if (name == "session") {
                attributes.emplace_back(CandleSession::parse(value).get());
            } else if (name == "alignment") {
                attributes.emplace_back(CandleAlignment::parse(value).get());
            } else if (name == "priceLevel") {
                attributes.emplace_back(CandlePriceLevel::parse(value));
            } else {
                std::cerr << "Invalid attribute: " << arg << '\n';
                printUsage();

                return 1;
            }
        }

        if (periodValue || periodType) {
            attributes.emplace_back(CandlePeriod::valueOf(
                periodValue.value_or(1.0), periodType ? CandleType::parse(*periodType).get() : CandleType::TICK));
        }

        const auto symbol = CandleSymbol::valueOf(baseSymbol, attributes);

        std::cout << "Subscribing to " << symbol.toString() << " from " << TimeFormat::DEFAULT.format(fromTime)
                  << std::endl;

        const auto endpoint = DXEndpoint::create()->connect(address);
        const auto subscription = endpoint->getFeed()->createSubscription(Candle::TYPE);

        // Listener must be attached before symbols are added.
        subscription->addEventListener<Candle>([](const auto &candles) {
            for (const auto &candle : candles) {
                std::cout << candle->toString() + "\n";
            }
        });

        // The history subscription: the candles from the specified time and then the updates.
        subscription->addSymbols(TimeSeriesSubscriptionSymbol(symbol, fromTime));

        std::cout << "Press enter to stop" << std::endl;
        std::cin.get();

        endpoint->closeAndAwaitTermination();
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
