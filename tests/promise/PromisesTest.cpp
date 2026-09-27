// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_set>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

// The scenarios follow DXFeedPromiseTest of the Java API: a LOCAL_HUB endpoint, the publisher observes the subscription
// created by the promises, and the promises complete when the matching events are published.
struct PromisesFixture : test::LocalHubFixture {
    std::mutex mutex{};
    std::deque<SymbolWrapper> added{};
    std::deque<SymbolWrapper> removed{};

    void trackSubscription(const EventTypeEnum &eventType) {
        publisher->getSubscription(eventType)->addChangeListener(ObservableSubscriptionChangeListener::create(
            [this](const std::unordered_set<SymbolWrapper> &symbols) {
                std::lock_guard lock{mutex};
                added.insert(added.end(), symbols.begin(), symbols.end());
            },
            [this](const std::unordered_set<SymbolWrapper> &symbols) {
                std::lock_guard lock{mutex};
                removed.insert(removed.end(), symbols.begin(), symbols.end());
            },
            [] {}));
    }

    static std::optional<SymbolWrapper> poll(std::deque<SymbolWrapper> &queue) {
        if (queue.empty()) {
            return std::nullopt;
        }

        auto symbol = queue.front();

        queue.pop_front();

        return symbol;
    }

    // The change listeners of the publisher subscription are notified through the endpoint executor.
    std::optional<SymbolWrapper> pollAdded() {
        process();
        std::lock_guard lock{mutex};

        return poll(added);
    }

    void assertAdded(const std::string &symbol) {
        process();
        std::lock_guard lock{mutex};
        const auto s = poll(added);

        REQUIRE(s.has_value());
        REQUIRE(s->asStringSymbol() == symbol);
        REQUIRE(added.empty());
        REQUIRE(removed.empty());
    }

    void assertRemoved(const std::string &symbol) {
        process();
        std::lock_guard lock{mutex};
        const auto s = poll(removed);

        REQUIRE(s.has_value());
        REQUIRE(s->asStringSymbol() == symbol);
        REQUIRE(added.empty());
        REQUIRE(removed.empty());
    }

    void assertRemovedTimeSeries(const CandleSymbol &symbol) {
        process();
        std::lock_guard lock{mutex};
        const auto s = poll(removed);

        REQUIRE(s.has_value());
        REQUIRE(s->isTimeSeriesSubscriptionSymbol());
        REQUIRE(s->asTimeSeriesSubscriptionSymbol()->getEventSymbol()->toStringUnderlying() == symbol.toString());
        REQUIRE(added.empty());
        REQUIRE(removed.empty());
    }

    void assertEmptyAddedAndRemoved() {
        process();
        std::lock_guard lock{mutex};

        REQUIRE(added.empty());
        REQUIRE(removed.empty());
    }

    void publishTrade(const std::string &symbol) {
        publishAndProcess(std::make_shared<Trade>(symbol));
    }
};

TEST_CASE_FIXTURE(PromisesFixture, "getLastEventPromise completes when the event is published") {
    trackSubscription(Trade::TYPE);

    const auto aPromise = feed->getLastEventPromise<Trade>("A");
    assertAdded("A");
    const auto bPromise = feed->getLastEventPromise<Trade>("B");
    assertAdded("B");
    const auto cPromise = feed->getLastEventPromise<Trade>("C");
    assertAdded("C");
    const auto bPromise2 = feed->getLastEventPromise<Trade>("B");
    assertEmptyAddedAndRemoved();

    REQUIRE_FALSE(aPromise->isDone());
    REQUIRE_FALSE(bPromise->isDone());
    REQUIRE_FALSE(cPromise->isDone());
    REQUIRE_FALSE(bPromise2->isDone());

    publishTrade("A");
    assertRemoved("A");
    REQUIRE(aPromise->isDone());
    REQUIRE(aPromise->getResult()->getEventSymbol() == "A");
    REQUIRE_FALSE(bPromise->isDone());
    REQUIRE_FALSE(cPromise->isDone());
    REQUIRE_FALSE(bPromise2->isDone());

    publishTrade("B");
    assertRemoved("B");
    REQUIRE(bPromise->isDone());
    REQUIRE(bPromise->getResult()->getEventSymbol() == "B");
    REQUIRE(bPromise2->isDone());
    REQUIRE(bPromise2->getResult()->getEventSymbol() == "B");
    REQUIRE_FALSE(cPromise->isDone());

    publishTrade("C");
    assertRemoved("C");
    REQUIRE(cPromise->isDone());
    REQUIRE(cPromise->getResult()->getEventSymbol() == "C");
}

TEST_CASE_FIXTURE(PromisesFixture, "A cancelled getLastEventPromise removes its subscription") {
    trackSubscription(Trade::TYPE);

    const auto promise = feed->getLastEventPromise<Trade>("T");
    assertAdded("T");
    REQUIRE_FALSE(promise->isDone());

    promise->cancel();
    assertRemoved("T");
    REQUIRE(promise->isDone());
    REQUIRE(promise->isCancelled());
}

TEST_CASE_FIXTURE(PromisesFixture, "getLastEventsPromises and Promises::allOf complete when all events are published") {
    const auto promises = feed->getLastEventsPromises<Quote>({"AAPL", "IBM"});
    const auto all = Promises::allOf(promises);

    REQUIRE_FALSE(all->isDone());

    publishAndProcess(std::make_shared<Quote>("AAPL")->withBidPrice(10.0).withAskPrice(11.0).sharedAs<Quote>());
    REQUIRE_FALSE(all->isDone());

    publishAndProcess(std::make_shared<Quote>("IBM")->withBidPrice(20.0).withAskPrice(21.0).sharedAs<Quote>());
    REQUIRE(all->awaitWithoutException(5s));

    std::vector<std::shared_ptr<Quote>> quotes{};

    for (const auto &promise : *promises) {
        quotes.push_back(promise.getResult());
    }

    REQUIRE(quotes.size() == 2);
    REQUIRE(quotes[0]->getEventSymbol() == "AAPL");
    REQUIRE(quotes[0]->getBidPrice() == 10.0);
    REQUIRE(quotes[0]->getAskPrice() == 11.0);
    REQUIRE(quotes[1]->getEventSymbol() == "IBM");
    REQUIRE(quotes[1]->getBidPrice() == 20.0);
    REQUIRE(quotes[1]->getAskPrice() == 21.0);
}

// Requires Graal SDK v3.5.0+: before it, dxfg_Promise_awaitWithoutException returned 0 ("completed") on timeout.
TEST_CASE_FIXTURE(PromisesFixture, "awaitWithoutException returns false and cancels the promise on timeout") {
    const auto promise = feed->getLastEventPromise<Quote>("NO-DATA");

    REQUIRE_FALSE(promise->isDone());
    REQUIRE_FALSE(promise->awaitWithoutException(100ms));
    REQUIRE(promise->isCancelled());
}

namespace {

constexpr auto SERIES_SYMBOL = "TEST";

std::shared_ptr<Series> createSeries(std::int64_t index, std::int32_t expiration, double volatility,
                                     std::int32_t eventFlags) {
    auto series = std::make_shared<Series>(SERIES_SYMBOL);

    series->setIndex(index);
    series->setExpiration(expiration);
    series->setVolatility(volatility);
    series->setEventFlags(eventFlags);

    return series;
}

void checkSeries(const std::shared_ptr<Series> &series, std::int64_t index, std::int32_t expiration,
                 double volatility) {
    REQUIRE(series->getEventSymbol() == SERIES_SYMBOL);
    REQUIRE(series->getIndex() == index);
    REQUIRE(series->getExpiration() == expiration);
    REQUIRE(series->getVolatility() == volatility);
    REQUIRE(series->getEventFlags() == 0);
}

} // namespace

TEST_CASE_FIXTURE(PromisesFixture, "getIndexedEventsPromise completes on the end of the snapshot") {
    // Series is a simple indexed event with plain delegate logic (unlike Order)
    trackSubscription(Series::TYPE);

    const auto promise = feed->getIndexedEventsPromise<Series>(SERIES_SYMBOL, IndexedEventSource::DEFAULT);
    assertAdded(SERIES_SYMBOL);
    REQUIRE_FALSE(promise->isDone());

    publishAndProcess(createSeries(3, 300, 10.01, EventFlag::SNAPSHOT_BEGIN.getFlag()));
    REQUIRE_FALSE(promise->isDone());
    publishAndProcess(createSeries(2, 200, 10.02, 0));
    REQUIRE_FALSE(promise->isDone());
    publishAndProcess(createSeries(1, 100, 10.03, 0));
    REQUIRE_FALSE(promise->isDone());
    publishAndProcess(
        createSeries(0, 0, math::NaN, static_cast<std::int32_t>((EventFlag::SNAPSHOT_END | EventFlag::REMOVE_EVENT).getMask())));
    assertRemoved(SERIES_SYMBOL);
    REQUIRE(promise->isDone());

    const auto list = promise->getResult();

    REQUIRE(list.size() == 3);
    checkSeries(list[0], 1, 100, 10.03);
    checkSeries(list[1], 2, 200, 10.02);
    checkSeries(list[2], 3, 300, 10.01);
}

namespace {

// A function, not a namespace-scope constant: CandleType::MINUTE is a static of the library and may be not initialized
// yet when the static initializers of this translation unit run.
CandleSymbol candleSymbol() {
    return CandleSymbol::valueOf("TEST", CandlePeriod::valueOf(1, CandleType::MINUTE));
}

// 20200116-120000-0500
constexpr std::int64_t CANDLE_TIME = 1579194000000LL;
constexpr std::int64_t CANDLE_PERIOD = 60'000LL;

std::shared_ptr<Candle> createCandle(std::int64_t time, std::int64_t count, std::int32_t eventFlags) {
    auto candle = std::make_shared<Candle>(candleSymbol());

    candle->setTime(time);
    candle->setCount(count);
    candle->setEventFlags(eventFlags);

    return candle;
}

void checkCandle(const std::shared_ptr<Candle> &candle, std::int64_t time, std::int64_t count) {
    INFO("candle: ", candle->toString());
    REQUIRE(candle->getEventSymbol() == candleSymbol());
    REQUIRE(candle->getTime() == time);
    REQUIRE(candle->getCount() == count);
    REQUIRE(candle->getEventFlags() == 0);
}

} // namespace

TEST_CASE_FIXTURE(PromisesFixture, "getTimeSeriesPromise completes when the requested time range is received") {
    // Candle is an intricate time series event with complex fetch time heuristic logic
    trackSubscription(Candle::TYPE);

    constexpr auto subTime = CANDLE_TIME - 2 * CANDLE_PERIOD;
    const auto promise = feed->getTimeSeriesPromise<Candle>(candleSymbol(), subTime, CANDLE_TIME);
    const auto addedSymbol = pollAdded();

    REQUIRE(addedSymbol.has_value());
    REQUIRE(addedSymbol->isTimeSeriesSubscriptionSymbol());

    const auto timeSeriesSymbol = addedSymbol->asTimeSeriesSubscriptionSymbol();

    REQUIRE(timeSeriesSymbol->getEventSymbol()->toStringUnderlying() == candleSymbol().toString());
    REQUIRE(timeSeriesSymbol->getFromTime() <= subTime);
    assertEmptyAddedAndRemoved();
    REQUIRE_FALSE(promise->isDone());

    publishAndProcess(createCandle(CANDLE_TIME, 0, EventFlag::SNAPSHOT_BEGIN.getFlag()));
    REQUIRE_FALSE(promise->isDone());
    publishAndProcess(createCandle(CANDLE_TIME - CANDLE_PERIOD, 1, 0));
    REQUIRE_FALSE(promise->isDone());
    publishAndProcess(createCandle(subTime, 2, 0));
    assertRemovedTimeSeries(candleSymbol());
    REQUIRE(promise->isDone());

    const auto list = promise->getResult();

    REQUIRE(list.size() == 3);
    checkCandle(list[0], subTime, 2);
    checkCandle(list[1], CANDLE_TIME - CANDLE_PERIOD, 1);
    checkCandle(list[2], CANDLE_TIME, 0);
}
