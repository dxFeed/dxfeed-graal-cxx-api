// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <format>

using namespace dxfcpp;
using namespace std::literals;

void testQuoteListener(const std::string &symbol) {
    // Creates a subscription attached to a default DXFeed with a Quote event type.
    // The endpoint address to use is stored in the "dxfeed.properties" file.
    const auto subscription = DXFeed::getInstance()->createSubscription(Quote::TYPE);

    // Listener must be attached before symbols are added.
    // Filtered by quotes
    subscription->addEventListener<Quote>([](const auto &quotes) {
        for (const auto &q : quotes) {
            std::cout << "Mid = " + std::to_string((q->getBidPrice() + q->getAskPrice()) / 2) + "\n";
        }
    });

    // Adds specified symbol.
    subscription->addSymbols(symbol);
}

void testQuotePromises(const std::string &symbol) {
    // Creates a subscription attached to a default DXFeed with a Quote event type.
    // The endpoint address to use is stored in the "dxfeed.properties" file.

    const auto promises = DXFeed::getInstance()->getLastEventsPromises<Quote>({symbol});

    // combine the list of promises into one with Promises utility method and wait
    Promises::allOf(*promises)->awaitWithoutException(std::chrono::seconds(5s));

    // now iterate the promises to retrieve results
    for (const auto &promise : *promises) {
        const auto quote = promise.getResult();

        std::cout << std::format("TQP: Mid = {}\n", std::to_string((quote->getBidPrice() + quote->getAskPrice()) / 2));
    }
}

void testQuoteAndTradeListener(const std::string &symbol) {
    // Creates a subscription attached to a default DXFeed with Quote and Trade event types.
    // The endpoint address to use is stored in the "dxfeed.properties" file.
    const auto subscription = DXFeed::getInstance()->createSubscription({Quote::TYPE, Trade::TYPE});

    // Listener must be attached before symbols are added.
    subscription->addEventListener([](const auto &events) {
        for (const auto &e : events) {
            // An example of using is<T> and sharedAs<T> (may return empty shared_ptr)
            if (const auto &q = e->template sharedAs<Quote>(); q) {
                std::cout << q->toString() + "\n";
            } else if (e->template is<Trade>()) {
                std::cout << e->template sharedAs<Trade>()->toString() + "\n";
            }
        }
    });

    // Another way to subscribe:
    // subscription->addEventListener<MarketEvent>([](const auto &events) {
    //     for (const auto &e : events) {
    //         std::cout << e->toString() + "\n";
    //     }
    // });

    // Adds specified symbol.
    subscription->addSymbols(symbol);
}

[[noreturn]] inline void testTradeSnapshots(const std::string &symbol) {
    const auto feed = DXFeed::getInstance();
    const auto sub = feed->createSubscription(Trade::TYPE);

    sub->addSymbols(symbol);

    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        std::cout << "LAST: " + feed->getLastEvent(std::make_shared<Trade>(symbol))->toString() + "\n";
        std::this_thread::sleep_for(1s);
    }
}

/*
 * Creates multiple event listener and subscribe to Quote and Trade events.
 * Use default DXFeed instance for that data feed address is defined by "dxfeed.properties" file.
 */
int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;

    try {
        if (argc < 2) {
            std::cout << R"(
Usage:
QuoteAndTradeSample <symbol>

Where:
    symbol - Is security symbol (e.g. IBM, AAPL, SPX etc.)
)";
            return 0;
        }

        // Specified instrument name, for example AAPL, IBM, MSFT, etc.
        const auto symbol = argv[1];

        // Disable QD logging.
        // Logging::init();

        testQuoteListener(symbol);
        testQuotePromises(symbol);
        testQuoteAndTradeListener(symbol);
        testTradeSnapshots(symbol);
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }
}
