// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

void testQuoteListener(const std::string &symbol) {
    using namespace dxfcpp;

    // Creates a subscription attached to a default DXFeed with a Quote event type.
    // The endpoint address to use is stored in the "dxfeed.properties" file.
    auto subscription = DXFeed::getInstance()->createSubscription(Quote::TYPE);

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

void testQuoteAndTradeListener(const std::string &symbol) {
    using namespace dxfcpp;

    // Creates a subscription attached to a default DXFeed with a Quote and Trade event types.
    // The endpoint address to use is stored in the "dxfeed.properties" file.
    auto subscription = DXFeed::getInstance()->createSubscription({Quote::TYPE, Trade::TYPE});

    // Listener must be attached before symbols are added.
    subscription->addEventListener([](const auto &events) {
        for (const auto &e : events) {
            //An example of using is<T> and sharedAs<T> (may return empty shared_ptr)
            if (const auto& q = e-> template sharedAs<Quote>(); q) {
                std::cout << "Q: " + q->toString() + "\n";
            } else if (e-> template is<Trade>()) {
                std::cout << "T: " + e-> template sharedAs<Trade>()->toString() + "\n";
            }
        }
    });

    // Adds specified symbol.
    subscription->addSymbols(symbol);
}

/*
 * Creates multiple event listener and subscribe to Quote and Trade events.
 * Use default DXFeed instance for that data feed address is defined by "dxfeed.properties" file.
 */
int main(int argc, char *argv[]) {
    using namespace dxfcpp;
    using namespace std::string_literals;

    if (argc < 2) {
        std::cout << R"(
Usage:
DxFeedSample <symbol>

Where:
    symbol - Is security symbol (e.g. IBM, AAPL, SPX etc.)
)";
        return 0;
    }

    // Specified instrument name, for example AAPL, IBM, MSFT, etc.
    auto symbol = argv[1];

    testQuoteListener(symbol);
    testQuoteAndTradeListener(symbol);

    std::cin.get();
}
