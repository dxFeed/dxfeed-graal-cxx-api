// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../src/promise/PromiseContext.hpp"
#include "../support/TestSupport.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

namespace {

bool contains(const std::string &text, std::string_view part) {
    return text.find(part) != std::string::npos;
}

// Runs the action that has to fail with a JavaException and returns the exception.
template <typename Action> JavaException captureJavaException(Action &&action) {
    try {
        action();
    } catch (const JavaException &e) {
        return e;
    }

    throw std::runtime_error("the JavaException was expected");
}

// The part of the text that does not depend on the measured time: "(timeout <N> ms) waiting for ...".
std::string withoutElapsed(const std::string &text) {
    const auto pos = text.find("(timeout");

    return pos == std::string::npos ? text : text.substr(pos);
}

} // namespace

// The text of QD's Promise.await(timeout) on a timeout is what the description of the request is attached to: if the
// SDK changes it, this test has to fail instead of silently losing the description.
TEST_CASE_FIXTURE(test::LocalHubFixture, "Promise timeout: a promise without a description keeps the SDK text") {
    const auto promises = feed->getLastEventsPromises<Quote>({"A", "B"});
    const auto all = Promises::allOf(promises);

    const auto e = captureJavaException([&] {
        all->await(100ms);
    });

    CHECK(contains(e.getJavaClassName(), "CancellationException"));
    CHECK(e.getJavaMessage() == "await timed out");
    CHECK_FALSE(contains(e.what(), "endpoint state"));
}

TEST_CASE("Promise timeout: the text names the request, the waiting time and the endpoint state") {
    const auto endpoint = DXEndpoint::create();
    const auto promise = endpoint->getFeed()->getLastEventPromise<Quote>("AAPL");

    const auto e = captureJavaException([&] {
        static_cast<void>(promise->await(100ms));
    });
    const std::string text = e.what();

    INFO(text);
    CHECK(contains(e.getJavaClassName(), "CancellationException"));
    CHECK(contains(e.getJavaMessage(), "await timed out; timed out after "));
    CHECK(contains(text, "getLastEventPromise"));
    CHECK(contains(text, "Quote"));
    CHECK(contains(text, "AAPL"));
    CHECK(contains(text, "100 ms"));
    CHECK(contains(text, "NOT_CONNECTED"));
    // The Java stack of the original exception is kept.
    CHECK_FALSE(e.getJavaStackTrace().empty());

    endpoint->close();
}

TEST_CASE("Promise timeout: the state of a connected endpoint") {
    using State = DXEndpoint::State;

    const test::LoopbackTcpServer server{};
    const auto executor = InPlaceExecutor::create();
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->withExecutor(executor)->build();

    endpoint->connect(server.getAddress());
    REQUIRE(test::waitUntil([&] {
        executor->processAllPendingTasks();

        return endpoint->getState() == State::CONNECTED;
    }));

    const auto promise = endpoint->getFeed()->getLastEventPromise<Quote>("AAPL");
    const auto e = captureJavaException([&] {
        static_cast<void>(promise->await(100ms));
    });

    INFO(e.what());
    CHECK(contains(e.what(), "endpoint state: CONNECTED"));

    endpoint->close();
}

TEST_CASE("Promise timeout: the time series request has the time range") {
    const auto endpoint = DXEndpoint::create();
    const auto promise = endpoint->getFeed()->getTimeSeriesPromise<Candle>(CandleSymbol::valueOf("TEST"), 1000,
                                                                           std::numeric_limits<std::int64_t>::max());

    const auto e = captureJavaException([&] {
        static_cast<void>(promise->await(100ms));
    });
    const std::string text = e.what();

    INFO(text);
    CHECK(contains(text, "getTimeSeriesPromise<Candle>(TEST"));
    CHECK(contains(text, "fromTime="));
    CHECK(contains(text, "toTime=max"));
    CHECK_FALSE(contains(text, "source="));

    endpoint->close();
}

TEST_CASE("Promise timeout: the indexed events request has the source") {
    const auto endpoint = DXEndpoint::create();
    const auto promise = endpoint->getFeed()->getIndexedEventsPromise<Order>("AAPL", OrderSource::ntv);

    const auto e = captureJavaException([&] {
        static_cast<void>(promise->await(100ms));
    });
    const std::string text = e.what();

    INFO(text);
    CHECK(contains(text, "getIndexedEventsPromise<Order>(AAPL"));
    CHECK(contains(text, "source=" + OrderSource::ntv.toString()));
    CHECK_FALSE(contains(text, "fromTime="));

    endpoint->close();
}

TEST_CASE("Promise timeout: each promise of the list describes its own symbol") {
    const auto endpoint = DXEndpoint::create();
    const auto promises = endpoint->getFeed()->getLastEventsPromises<Quote>({"A", "B"});

    REQUIRE(promises->size() == 2);

    const auto e0 = captureJavaException([&] {
        static_cast<void>(promises->at(0).await(100ms));
    });
    const auto e1 = captureJavaException([&] {
        static_cast<void>(promises->at(1).await(100ms));
    });

    INFO(e0.what());
    INFO(e1.what());
    CHECK(contains(e0.what(), "getLastEventsPromises<Quote>(A)"));
    CHECK(contains(e1.what(), "getLastEventsPromises<Quote>(B)"));

    endpoint->close();
}

TEST_CASE("Promise timeout: both overloads of await give the same text") {
    const auto endpoint = DXEndpoint::create();
    const auto feed = endpoint->getFeed();
    const auto promise1 = feed->getLastEventPromise<Quote>("AAPL");
    const auto promise2 = feed->getLastEventPromise<Quote>("AAPL");

    const auto e1 = captureJavaException([&] {
        static_cast<void>(promise1->await(std::int32_t{100}));
    });
    const auto e2 = captureJavaException([&] {
        static_cast<void>(promise2->await(100ms));
    });

    CHECK(withoutElapsed(e1.what()) == withoutElapsed(e2.what()));
    CHECK(contains(e1.what(), "(timeout 100 ms)"));

    endpoint->close();
}

TEST_CASE("Promise timeout: a cancelled promise is not described") {
    const auto endpoint = DXEndpoint::create();
    const auto feed = endpoint->getFeed();
    const auto promise = feed->getLastEventPromise<Quote>("AAPL");

    promise->cancel();

    for (const auto timeout : {1000ms, 0ms}) {
        const auto e = captureJavaException([&] {
            static_cast<void>(promise->await(timeout));
        });
        const std::string text = e.what();

        INFO(text);
        CHECK(contains(e.getJavaClassName(), "CancellationException"));
        CHECK_FALSE(contains(text, "timed out"));
        CHECK_FALSE(contains(text, "getLastEventPromise"));
        CHECK_FALSE(contains(text, "endpoint state"));
    }

    endpoint->close();
}

TEST_CASE("Promise timeout: closing the endpoint does not complete the promise, the timeout names the state CLOSED") {
    const auto endpoint = DXEndpoint::create();
    const auto promise = endpoint->getFeed()->getLastEventPromise<Quote>("AAPL");

    endpoint->close();

    const auto e = captureJavaException([&] {
        static_cast<void>(promise->await(100ms));
    });
    const std::string text = e.what();

    INFO(text);
    CHECK(contains(e.getJavaClassName(), "CancellationException"));
    CHECK(contains(text, "getLastEventPromise<Quote>(AAPL)"));
    CHECK(contains(text, "endpoint state: CLOSED"));
}

TEST_CASE("Promise timeout: a moved promise and the result of sharedAs keep the description") {
    const auto endpoint = DXEndpoint::create();
    const auto feed = endpoint->getFeed();
    const auto promise = feed->getLastEventPromise<Quote>("AAPL");
    const auto shared = promise->sharedAs<MarketEvent>();
    const auto promiseToMove = feed->getLastEventPromise<Quote>("AAPL");
    Promise<std::shared_ptr<Quote>> moved{std::move(*promiseToMove)};

    const auto sharedException = captureJavaException([&] {
        static_cast<void>(shared->await(100ms));
    });

    INFO(sharedException.what());
    CHECK(contains(sharedException.what(), "getLastEventPromise<Quote>(AAPL)"));

    const auto movedException = captureJavaException([&] {
        static_cast<void>(moved.await(100ms));
    });

    INFO(movedException.what());
    CHECK(contains(movedException.what(), "getLastEventPromise<Quote>(AAPL)"));
    CHECK(contains(movedException.what(), "endpoint state: NOT_CONNECTED"));

    endpoint->close();
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "Promise timeout: a promise completed in time returns the result") {
    const auto promise = feed->getLastEventPromise<Quote>("AAPL");

    publishAndProcess(std::make_shared<Quote>("AAPL")->withBidPrice(10.0).withAskPrice(11.0).sharedAs<Quote>());

    const auto quote = promise->await(5s);

    REQUIRE(quote != nullptr);
    CHECK(quote->getBidPrice() == 10.0);
}

TEST_CASE("Promise timeout: describePromiseTimeout") {
    PromiseContext context{};

    context.method = "getLastEventPromise";
    context.eventType = "Quote";
    context.symbol = "AAPL";

    SUBCASE("the endpoint is gone") {
        // A default-constructed weak pointer is expired.
        CHECK(describePromiseTimeout(context, 150, 100) == "timed out after 150 ms (timeout 100 ms) waiting for "
                                                           "getLastEventPromise<Quote>(AAPL); endpoint state: unknown");
    }

    SUBCASE("the endpoint is alive") {
        const auto endpoint = DXEndpoint::create();

        context.endpoint = endpoint;

        CHECK(contains(describePromiseTimeout(context, 150, 100), "endpoint state: NOT_CONNECTED"));

        endpoint->close();
    }

    SUBCASE("the source and the time range") {
        context.source = "NTV";
        context.fromTime = 0;
        context.toTime = std::numeric_limits<std::int64_t>::max();

        const auto text = describePromiseTimeout(context, 1, 2);

        CHECK(contains(text, "(AAPL, source=NTV, fromTime="));
        CHECK(contains(text, ", toTime=max); endpoint state: unknown"));
    }
}
