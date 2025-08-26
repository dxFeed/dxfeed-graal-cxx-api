// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("getLastEventsPromises & getLastEventPromise test") {
    // Disable QD logging.
    Logging::init();

    const auto endpoint = DXEndpoint::create()->connect("demo.dxfeed.com:7300");
    const auto promises = endpoint->getFeed()->getLastEventsPromises<Quote>({"AAPL&Q", "IBM&Q"});

    // combine the list of promises into one with Promises utility method and wait
    Promises::allOf(promises)->awaitWithoutException(std::chrono::seconds(5s));

    // now iterate the promises to retrieve results
    for (const auto &promise : *promises) {
        auto r = promise.getResult();

        std::cout << r << std::endl;
    }

    std::vector<std::shared_ptr<Promise<std::shared_ptr<Quote>>>> promises2{};

    for (auto s : {"AAPL&Q", "IBM&Q"}) {
        promises2.push_back(endpoint->getFeed()->getLastEventPromise<Quote>(s));
    }

    // ignore timeout and continue to print retrieved quotes even on timeout
    const auto ok = Promises::allOf(promises2)->awaitWithoutException(std::chrono::seconds(5));
    ignoreUnused(ok);

    for (const auto &promise : promises2) {
        auto quote = promise->getResult();

        std::cout << quote << std::endl;
    }
}