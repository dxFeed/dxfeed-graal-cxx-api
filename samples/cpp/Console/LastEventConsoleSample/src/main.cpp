// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

/**
 * This sample demonstrates a way to subscribe to the big world of symbols with dxFeed API, so that the events are
 * updated and cached in memory of this process, and then take snapshots of those events from memory whenever
 * they are needed. This example repeatedly reads symbol name from the console and prints a snapshot of its last
 * quote, trade, summary, and profile events.
 */
int main() {
    using namespace dxfcpp;
    using namespace std::literals;

    try {
        /*
         * Permanent subscription to the world is performed with a special property named "dxfeed.qd.subscribe.ticker".
         * Its value consists of a comma-separated list of records, followed by a space, followed by a comma-separated
         * list of symbols. Record names for composite (NBBO) events are the same as the corresponding event classes
         * in API. The string below defines subscription for quote, trade, summary, and profile composite events:
         */
        const auto records = "Quote,Trade,Summary,Profile";

        /*
         * Records for regional-exchange events are derived by appending "&" (ampersand) and the a single-digit
         * exchange code. Regexp-like syntax can be used instead of listing them all. For example, the commented
         * line below and to the mix a subscription on regional quotes from all potential exchange codes A-Z
         */
        // auto records = "Quote,Trade,Summary,Profile,Quote&[A-Z]";
        /*
         * There is an important trade-off between a resource consumption and speed of access to the last events.
         * The whole world of stocks and options from all the exchanges is very large and will consume gigabytes
         * of memory to cache. Moreover, this cache has to be constantly kept up-to-date which consumes a lot of
         * network and CPU.
         *
         * A particular application's uses cases has to be studied to figure out what is option for this particular
         * application. If some events are known be rarely needed and a small delay while access them can be
         * tolerated, then it is not worth configuring a permanent subscription for them. The code in this
         * sample works using DXFeed::getLastEventPromise method that will request event from upstream data provider
         * if it is not present in local in-memory cache.
         */

        /*
         * There are multiple ways to specify a list of symbols. It is typically taken from IPF file and its
         * specification consists of an URL to the file which has to contain ".ipf" in order to be recognized.
         * The string below defines subscription for all symbols that are available on the demo feed.
         */
        const auto symbols = "http://dxfeed.s3.amazonaws.com/masterdata/ipf/demo/mux-demo.ipf.zip";

        /*
         * Permanent subscription property "dxfeed.qd.subscribe.ticker" can be directly placed into the
         * "dxfeed.properties" file and no custom DXEndpoint instance will be needed. Here it is explicitly
         * specified using a DXFeedEndpoint::Builder class. Note, that we don't use "connect" method on DXEndpoint.
         * It is assumed by this sample that "dxfeed.address" property is specified in "dxfeed.properties" and
         * connection is automatically established to that address.
         * Alternatively, "dxfeed.address" can also be specified in the builder.
         */
        const auto endpoint =
            DXEndpoint::newBuilder()->withProperty("dxfeed.qd.subscribe.ticker", records + " "s + symbols)->build();

        /*
         * The actual client code does not need a reference to DXEndpoint, which only contains lifecycle
         * methods like "connect" and "close". The client code needs a reference to DXFeed.
         */
        auto feed = endpoint->getFeed();

        /*
         * Print a short help.
         */
        std::cout << "Type symbols to get their quote, trade, summary, and profile event snapshots\n";

        while (true) {
            /*
             * User of this sample application can type symbols on the console. Symbol like "IBM" corresponds
             * to the stock. Symbol like "IBM&N" corresponds to the information from a specific exchange.
             * See the dxFeed Symbol guide at http://www.dxfeed.com/downloads/documentation/dxFeed_Symbol_Guide.pdf
             */
            std::string symbol;

            std::getline(std::cin, symbol);

            /*
             * The first step is to extract promises for all events that we are interested in. This way we
             * can get an event even if we have not previously subscribed for it.
             */
            auto quotePromise = feed->getLastEventPromise<Quote>(symbol);
            auto tradePromise = feed->getLastEventPromise<Trade>(symbol);
            auto summaryPromise = feed->getLastEventPromise<Summary>(symbol);

            /*
             * All promises are put into a list for convenience.
             */
            std::vector<std::shared_ptr<Promise<std::shared_ptr<EventType>>>> promises{};

            promises.push_back(quotePromise->sharedAs<EventType>());
            promises.push_back(tradePromise->sharedAs<EventType>());
            promises.push_back(summaryPromise->sharedAs<EventType>());

            /*
             * Profile events are composite-only. They are not available for regional symbols like
             * "IBM&N" and the attempt to retrieve never completes (will time out), so we don't even try.
             */
            if (!MarketEventSymbols::hasExchangeCode(symbol)) {
                auto profilePromise = feed->getLastEventPromise<Profile>(symbol);

                promises.push_back(profilePromise->sharedAs<EventType>(true));
            }

            /*
             * If the events are available in the in-memory cache, then the promises will be completed immediately.
             * Otherwise, a request to the upstream data provider is sent. Below we combine promises using
             * Promises utility class from DXFeed API in order to wait for at most 1 second for all the
             * promises to complete. The last event promise never completes exceptionally, and we don't
             * have to specially process a case of timeout, so "awaitWithoutException" is used to continue
             * normal execution even on timeout. This sample prints a special message in the case of timeout.
             */
            if (!Promises::allOf(promises)->awaitWithoutException(1s)) {
                std::cout << "Request timed out\n";
            }

            /*
             * The combination above is used only to ensure a common wait of 1 second. Promises to individual events
             * are completed independently and the corresponding events can be accessed even if some events were not
             * available for any reason and the wait above had timed out. This sample just prints all results.
             * "<null>" is printed when the event is not available.
             */
            for (const auto &promise : promises) {
                auto event = promise->getResult();

                std::cout << (!event ? String::NUL : event->toString()) << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::days(365));
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
