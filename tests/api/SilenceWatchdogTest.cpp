// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

namespace {

constexpr auto TIMEOUT = 300ms;

// A PUBLISHER on the loopback interface and a FEED connected to it over TCP (as in DataIntegrityTest).
struct LoopbackFeed {
    std::shared_ptr<DXEndpoint> publisherEndpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER);
    std::shared_ptr<DXEndpoint> feedEndpoint = DXEndpoint::create(DXEndpoint::Role::FEED);
    std::string port = std::to_string(test::findFreeLoopbackPort());
    double price = 1.0;

    LoopbackFeed() {
        // A FEED refused before the PUBLISHER listens reconnects only after 10-20 s.
        publisherEndpoint->connect(":" + port + "[bindAddr=127.0.0.1]");
        REQUIRE(test::waitUntil([&] {
            return publisherEndpoint->getState() == DXEndpoint::State::CONNECTED;
        }));
        connectFeed();
    }

    // reconnect() of the FEED waits a random delay of up to 10 s; connect() after disconnect() does not.
    void connectFeed() {
        feedEndpoint->connect("127.0.0.1:" + port);
        REQUIRE(test::waitUntil([&] {
            return feedEndpoint->getState() == DXEndpoint::State::CONNECTED;
        }));
    }

    ~LoopbackFeed() {
        feedEndpoint->close();
        publisherEndpoint->close();
    }

    LoopbackFeed(const LoopbackFeed &) = delete;
    LoopbackFeed &operator=(const LoopbackFeed &) = delete;

    void publishQuote(const std::string &symbol) {
        const auto quote = std::make_shared<Quote>(symbol);

        // A new price for each quote: an unchanged one could be conflated.
        quote->setBidPrice(price);
        price += 1.0;
        publisherEndpoint->getPublisher()->publishEvents(quote);
    }
};

// What a watched subscription received; shared with onSilence, which may run on the watchdog thread after a test.
struct Watched {
    std::atomic<int> quotes{0};
    std::atomic<int> silences{0};
    std::mutex mutex{};
    std::vector<std::chrono::milliseconds> silenceDurations{};
    // Not owned: the subscription owns onSilence, which owns this.
    std::vector<const DXFeedSubscription *> silentSubscriptions{};

    static DXFeedSubscription::OnSilenceHandler handler(const std::shared_ptr<Watched> &self) {
        return [self](const std::shared_ptr<DXFeedSubscription> &sub, std::chrono::milliseconds silence) {
            {
                std::lock_guard lock{self->mutex};

                self->silenceDurations.push_back(silence);
                self->silentSubscriptions.push_back(sub.get());
            }

            ++self->silences;
        };
    }
};

std::shared_ptr<DXFeedSubscription> createQuoteSubscription(const std::shared_ptr<DXFeed> &feed,
                                                            const std::shared_ptr<Watched> &watched,
                                                            const std::string &symbol) {
    auto sub = feed->createSubscription(Quote::TYPE);

    sub->addEventListener<Quote>([watched](const auto &quotes) {
        watched->quotes += static_cast<int>(quotes.size());
    });
    sub->addSymbols(symbol);

    return sub;
}

// Publishes quotes until the subscription receives one: the subscription reaches the publisher asynchronously.
void publishUntilReceived(LoopbackFeed &loopback, const Watched &watched, const std::string &symbol) {
    const auto before = watched.quotes.load();

    REQUIRE(test::waitUntil(
        [&] {
            loopback.publishQuote(symbol);

            return watched.quotes.load() > before;
        },
        10s, 50ms));
}

std::vector<ErrorInfo> getSilenceWarnings() {
    std::vector<ErrorInfo> result{};

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.level == ErrorInfo::Level::WARN && error.component == "DXFeedSubscription" &&
            error.message.find("silence watchdog") != std::string::npos) {
            result.push_back(error);
        }
    }

    return result;
}

} // namespace

TEST_CASE("DXFeedSubscription :: silence watchdog :: fires once per silence and re-arms on the next batch") {
    const test::ErrorManagerGuard errorManagerGuard{};
    LoopbackFeed loopback{};
    const auto watched = std::make_shared<Watched>();
    const auto sub = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-1");

    // The first quote before the watchdog: a slow start must not look like a silence.
    publishUntilReceived(loopback, *watched, "SILENCE-1");
    sub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));

    REQUIRE(test::waitUntil([&] {
        return watched->silences.load() == 1;
    }));

    // Once per silence.
    std::this_thread::sleep_for(3 * TIMEOUT);
    CHECK(watched->silences.load() == 1);

    {
        std::lock_guard lock{watched->mutex};

        CHECK(watched->silenceDurations.front() >= TIMEOUT);
        CHECK(watched->silentSubscriptions.front() == sub.get());
    }

    auto warnings = getSilenceWarnings();

    REQUIRE(warnings.size() == 1);
    CHECK(warnings[0].message.find("DXFeedSubscription #") != std::string::npos);
    CHECK(warnings[0].message.find("[Quote]") != std::string::npos);
    CHECK(warnings[0].message.find("the timeout is 300 ms") != std::string::npos);

    // The next quote arms the watchdog again.
    publishUntilReceived(loopback, *watched, "SILENCE-1");

    REQUIRE(test::waitUntil([&] {
        return watched->silences.load() == 2;
    }));

    CHECK(getSilenceWarnings().size() == 2);

    sub->close();
}

TEST_CASE("DXFeedSubscription :: silence watchdog :: a new connection or a new call arms it again") {
    const test::ErrorManagerGuard errorManagerGuard{};
    LoopbackFeed loopback{};
    const auto watched = std::make_shared<Watched>();
    const auto sub = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-6");

    publishUntilReceived(loopback, *watched, "SILENCE-6");
    sub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));

    REQUIRE(test::waitUntil([&] {
        return watched->silences.load() == 1;
    }));

    // A new connection: a new silence.
    loopback.feedEndpoint->disconnect();
    REQUIRE(test::waitUntil([&] {
        return loopback.feedEndpoint->getState() == DXEndpoint::State::NOT_CONNECTED;
    }));
    loopback.connectFeed();

    REQUIRE(test::waitUntil([&] {
        return watched->silences.load() == 2;
    }));

    // A new call replaces the function: only the new one is called.
    const auto replaced = std::make_shared<Watched>();

    sub->setSilenceWatchdog(TIMEOUT, replaced->handler(replaced));

    REQUIRE(test::waitUntil([&] {
        return replaced->silences.load() == 1;
    }));

    std::this_thread::sleep_for(3 * TIMEOUT);

    CHECK(watched->silences.load() == 2);
    CHECK(replaced->silences.load() == 1);

    sub->close();
}

TEST_CASE("DXFeedSubscription :: silence watchdog :: batches keep it quiet, also without event listeners") {
    const test::ErrorManagerGuard errorManagerGuard{};
    LoopbackFeed loopback{};
    const auto watched = std::make_shared<Watched>();
    // A subscription with a listener tells when the quotes of the symbol reach the feed.
    const auto probe = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-2");
    const auto silent = std::make_shared<Watched>();
    const auto sub = loopback.feedEndpoint->getFeed()->createSubscription(Quote::TYPE);

    publishUntilReceived(loopback, *watched, "SILENCE-2");

    // No event listener: the watchdog itself receives the batches. Without a listener it is turned on before the
    // symbols, as an event listener is added. A longer timeout: a stall of a loaded host must not look like a silence.
    constexpr auto quietTimeout = 1s;

    sub->setSilenceWatchdog(quietTimeout, silent->handler(silent));
    sub->addSymbols("SILENCE-2");

    const auto publishUntil = std::chrono::steady_clock::now() + 2 * quietTimeout;

    while (std::chrono::steady_clock::now() < publishUntil) {
        loopback.publishQuote("SILENCE-2");
        std::this_thread::sleep_for(50ms);
    }

    CHECK(silent->silences.load() == 0);

    // The quotes stop: a silence.
    REQUIRE(test::waitUntil([&] {
        return silent->silences.load() == 1;
    }));

    // Too late without a listener: as addEventListener(), it throws.
    const auto late = loopback.feedEndpoint->getFeed()->createSubscription(Quote::TYPE);

    late->addSymbols("SILENCE-2");
    CHECK_THROWS_AS(late->setSilenceWatchdog(TIMEOUT, silent->handler(silent)), JavaException);

    late->close();
    sub->close();
    probe->close();
}

TEST_CASE("DXFeedSubscription :: silence watchdog :: off by default and with a zero timeout") {
    const test::ErrorManagerGuard errorManagerGuard{};
    LoopbackFeed loopback{};
    const auto watched = std::make_shared<Watched>();
    const auto byDefault = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-3");
    const auto turnedOff = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-3");

    publishUntilReceived(loopback, *watched, "SILENCE-3");
    turnedOff->setSilenceWatchdog(TIMEOUT, watched->handler(watched));
    turnedOff->setSilenceWatchdog(0ms, watched->handler(watched));

    std::this_thread::sleep_for(3 * TIMEOUT);

    CHECK(watched->silences.load() == 0);
    CHECK(getSilenceWarnings().empty());

    byDefault->close();
    turnedOff->close();
}

TEST_CASE("DXFeedSubscription :: silence watchdog :: not watched without a connected endpoint or after close") {
    const test::ErrorManagerGuard errorManagerGuard{};
    const auto watched = std::make_shared<Watched>();

    // A LOCAL_HUB endpoint does not connect.
    const test::LocalHubFixture localHub{};

    REQUIRE(localHub.endpoint->getState() != DXEndpoint::State::CONNECTED);

    const auto localSub = localHub.feed->createSubscription(Quote::TYPE);

    localSub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));
    localSub->addSymbols("SILENCE-4");

    // Attached to no feed.
    const auto detachedSub = DXFeedSubscription::create(Quote::TYPE);

    detachedSub->addSymbols("SILENCE-4");
    detachedSub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));

    // Closed after the watchdog is on.
    LoopbackFeed loopback{};
    const auto closedSub = loopback.feedEndpoint->getFeed()->createSubscription(Quote::TYPE);

    closedSub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));
    closedSub->addSymbols("SILENCE-4");
    closedSub->close();

    // Turned on after close: nothing.
    closedSub->setSilenceWatchdog(TIMEOUT, watched->handler(watched));

    std::this_thread::sleep_for(3 * TIMEOUT);

    CHECK(watched->silences.load() == 0);
    CHECK(getSilenceWarnings().empty());

    localSub->close();
    detachedSub->close();
}

TEST_CASE("DXFeedSubscription :: silence watchdog :: onSilence may close the subscription or throw") {
    const test::ErrorManagerGuard errorManagerGuard{};
    LoopbackFeed loopback{};
    const auto watched = std::make_shared<Watched>();
    const auto closing = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-5");
    const auto throwing = createQuoteSubscription(loopback.feedEndpoint->getFeed(), watched, "SILENCE-5");
    const auto closes = std::make_shared<std::atomic<int>>(0);
    const auto throws = std::make_shared<std::atomic<int>>(0);

    publishUntilReceived(loopback, *watched, "SILENCE-5");

    closing->setSilenceWatchdog(TIMEOUT, [closes](const std::shared_ptr<DXFeedSubscription> &sub, auto) {
        ++*closes;
        sub->close();
    });
    throwing->setSilenceWatchdog(TIMEOUT, [throws](const auto &, auto) {
        ++*throws;

        throw std::runtime_error("onSilence of the test failed");
    });

    REQUIRE(test::waitUntil([&] {
        return closes->load() == 1 && throws->load() == 1;
    }));
    std::this_thread::sleep_for(3 * TIMEOUT);

    CHECK(closes->load() == 1);
    CHECK(closing->isClosed());
    // Once per silence, even when onSilence throws.
    CHECK(throws->load() == 1);

    bool exceptionReported = false;

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.component == "DXFeedSubscription" &&
            error.message.find("onSilence of the test failed") != std::string::npos) {
            exceptionReported = true;
        }
    }

    CHECK(exceptionReported);

    throwing->close();
}
