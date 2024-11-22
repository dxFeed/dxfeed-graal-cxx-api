// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <iostream>
#include <thread>

/*
 * A sample that uses dxFeed on-demand history data replay service API show Accenture symbol "ACN" drops
 * under $1 on May 6, 2010 "Flashcrash" from 14:47:48 to 14:48:02 EST.
 */
int main(int /* argc */, char ** /* argv */) {
    using namespace dxfcpp;
    using namespace std::literals;

    try {
        // get on-demand-only data feed
        auto onDemand = OnDemandService::getInstance();
        auto feed = onDemand->getEndpoint()->getFeed();

        // subscribe to Accenture symbol ACN to print its quotes
        auto sub = feed->createSubscription(Quote::TYPE);

        sub->addEventListener<Quote>([](auto &&events) {
            for (auto &&quote : events) {
                std::cout << dxfcpp::formatTimeStampWithMillisWithTimeZone(quote->getEventTime()) + " : " +
                                 quote->getEventSymbol() + " bid " + dxfcpp::toString(quote->getBidPrice()) +
                                 " / ask " + dxfcpp::toString(quote->getAskPrice())
                          << std::endl;
            }
        });

        sub->addSymbols("ACN");

        // Watch Accenture drop under $1 on May 6, 2010 "Flashcrash" from 14:47:48 to 14:48:02 EST
        auto from = dxfcpp::TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse("2010-05-06 14:47:48.000 EST");
        auto to = dxfcpp::TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.parse("2010-05-06 14:48:02.000 EST");

        // switch into historical on-demand data replay mode
        onDemand->replay(from);

        // replaying events until end time reached
        while (onDemand->getTime() < to) {
            std::cout << "Current state is "s + DXEndpoint::stateToString(onDemand->getEndpoint()->getState()) + "," +
                             " on-demand time is " + dxfcpp::formatTimeStampWithMillisWithTimeZone(onDemand->getTime())
                      << std::endl;

            std::this_thread::sleep_for(1s);
        }

        // close endpoint completely to release resources and shutdown GraalVM
        onDemand->getEndpoint()->closeAndAwaitTermination();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
