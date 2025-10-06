// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>
#include <memory>

using namespace dxfcpp;
using namespace std::literals;

int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;

    try {
        if (argc < 5) {
            std::cout << R"(
Usage:
OptionChainSample <ipf-file> <symbol> <strikes-number> <months-number>

Where:
    <ipf-file>       - a name of instrument profiles file
    <symbol>         - the product or underlying symbol
    <strikes-number> - a number of strikes to print for each series
    <months-number>  - a number of months to print
Example:
    OptionChainSample http://dxfeed.s3.amazonaws.com/masterdata/ipf/demo/mux-demo.ipf.zip IBM 10 3
)";
            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        auto ipfFile = argv[1];
        // Specified instrument name, for example AAPL, IBM, MSFT, etc.
        auto symbol = argv[2];
        std::size_t strikesNumber = std::stoull(argv[3]);
        std::size_t monthsNumber = std::stoull(argv[4]);

        auto feed = DXFeed::getInstance();

        // subscribe to trade to learn an instrument last price
        std::cout << "Waiting for price of "s + symbol + " ...\n";
        const auto trade = feed->getLastEventPromise<Trade>(symbol)->await(1s);

        const auto price = trade->getPrice();

        std::cout << "Price of "s + symbol + " is " + toString(price) + "\n";
        std::cout << "Reading instruments from "s + ipfFile + "\n";

        auto instruments = InstrumentProfileReader::create()->readFromFile(ipfFile);

        std::cout << "Building option chains ...\n";
        auto chains = OptionChainsBuilder<InstrumentProfile>::build(instruments).getChains();
        auto chain = chains.at(symbol);

        monthsNumber = std::min(monthsNumber, chain.getSeries().size());

        std::vector<OptionSeries<InstrumentProfile>> seriesVector{};

        std::size_t counter = 0;

        for (const auto &series : chain.getSeries()) {
            if (counter >= monthsNumber) {
                break;
            }
            seriesVector.push_back(series);
            ++counter;
        }

        std::cout << "Requesting option quotes ...\n";

        std::unordered_map<std::shared_ptr<InstrumentProfile>, std::shared_ptr<Promise<std::shared_ptr<Quote>>>>
            quotes{};

        for (const auto &series : seriesVector) {
            for (auto strikes = series.getNStrikesAround(strikesNumber, price); auto strike : strikes) {
                if (series.getCalls().contains(strike)) {
                    auto call = series.getCalls().at(strike);

                    quotes[call] = feed->getLastEventPromise<Quote>(call->getSymbol());
                }

                if (series.getPuts().contains(strike)) {
                    auto put = series.getPuts().at(strike);

                    quotes[put] = feed->getLastEventPromise<Quote>(put->getSymbol());
                }
            }
        }

        std::vector<std::shared_ptr<Promise<std::shared_ptr<Quote>>>> promises{};

        // ReSharper disable once CppUseElementsView
        for (const auto &[ip, promise] : quotes) {
            promises.push_back(promise);
        }

        // ignore timeout and continue to print retrieved quotes even on timeout
        auto ok = Promises::allOf(promises)->awaitWithoutException(1s);
        ignoreUnused(ok);

        std::cout << "Printing option series ...\n";

        for (const auto &series : seriesVector) {
            std::cout << "Option series " + series.toString() << "\n";
            std::printf("    %10s %10s %10s %10s %10s\n", "C.BID", "C.ASK", "STRIKE", "P.BID", "P.ASK");

            for (const auto &strike : series.getNStrikesAround(strikesNumber, price)) {
                auto callQuote = std::make_shared<Quote>();
                auto putQuote = std::make_shared<Quote>();

                if (series.getCalls().contains(strike)) {
                    callQuote = quotes.at(series.getCalls().at(strike))->getResult();
                }

                if (series.getPuts().contains(strike)) {
                    putQuote = quotes.at(series.getPuts().at(strike))->getResult();
                }

                std::printf("    %10.3f %10.3f %10.3f %10.3f %10.3f\n", callQuote->getBidPrice(),
                            callQuote->getAskPrice(), strike, putQuote->getAskPrice(), putQuote->getBidPrice());
            }
        }
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
