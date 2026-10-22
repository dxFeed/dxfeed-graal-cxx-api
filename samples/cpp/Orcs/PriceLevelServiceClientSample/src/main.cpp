// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

using namespace dxfcpp;
using namespace std::literals;

void printUsage() {
    const auto usageString = R"(
Usage:
PriceLevelServiceClientSample <address> <symbol> <source> <from> <to> [--validate] [--verbose]

Where:
    address    - The RMI address of the ORCS server. To pass an authorization token, add "[login=entitle:<token>]" to
                 the address: "<address>[login=entitle:<token>]".
    symbol     - The candle symbol to retrieve, for example "AAPL{=d,gr=s}" (the "gr" attribute is the granularity of
                 the price levels in seconds).
    source     - The order source to retrieve, for example "NTV".
    from       - Is from-time for the request in standard formats.
                 Same examples of valid from-time:
                     20070101-123456
                     20070101-123456.123
                     2005-12-31 21:00:00
                     2005-12-31 21:00:00.123+03:00
                     2005-12-31 21:00:00.123+0400
                     2007-11-02Z
                     123456789 - value-in-milliseconds
    to         - Is to-time for the request in standard formats.
    --validate - Validate the received price levels with PriceLevelChecker (the details go to the QD log).
    --verbose  - Print the available symbols of each authorized order source.

Examples:
    PriceLevelServiceClientSample "orcs.example.com:7777[login=entitle:<token>]" "AAPL{=d,gr=s}" NTV 20260110-100000 20260110-230000 --validate)";

    std::cout << usageString << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 6) {
            printUsage();

            return 0;
        }

        // Parse args.
        const auto address = argv[1];
        const auto symbol = CandleSymbol::valueOf(argv[2]);
        const auto source = OrderSource::valueOf(argv[3]);
        const auto fromTime = TimeFormat::DEFAULT.parse(argv[4]);
        const auto toTime = TimeFormat::DEFAULT.parse(argv[5]);
        auto validate = false;
        auto verbose = false;

        for (int i = 6; i < argc; i++) {
            if (argv[i] == "--validate"sv) {
                validate = true;
            } else if (argv[i] == "--verbose"sv) {
                verbose = true;
            }
        }

        // Disable QD logging, unless PriceLevelChecker logs the validation to it.
        if (!validate) {
            Logging::init();
        }

        const auto service = PriceLevelService::create(address);
        const auto symbolsBySource = service->getAuthOrderSource()->getByOrderSources();

        std::cout << "Authorized sources: [";

        for (auto first = true; const auto &[orderSource, symbols] : symbolsBySource) {
            std::cout << (first ? "" : ", ") << orderSource.toString();
            first = false;
        }

        std::cout << "]" << std::endl;

        if (verbose) {
            for (const auto &[orderSource, symbols] : symbolsBySource) {
                std::cout << orderSource.toString() << ": [";

                for (auto first = true; const auto &s : symbols) {
                    std::cout << (first ? "" : ", ") << s;
                    first = false;
                }

                std::cout << "]" << std::endl;
            }
        }

        auto start = std::chrono::steady_clock::now();
        const auto orders = service->getOrders(symbol, source, fromTime, toTime, "sample");

        std::cout
            << "Received " << orders.size() << " orders in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
            << " ms" << std::endl;

        if (validate) {
            const auto valid = PriceLevelChecker::validate(orders, std::chrono::minutes(1), true);

            std::cout << "The orders are " << (valid ? "valid" : "not valid") << std::endl;
        }

        start = std::chrono::steady_clock::now();
        const auto quotes =
            service->getQuotes(CandleSymbol::valueOf(symbol.getBaseSymbol() + "{gr=s}"), fromTime, toTime, "sample");

        std::cout
            << "Received " << quotes.size() << " quotes in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
            << " ms" << std::endl;

        service->close();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
