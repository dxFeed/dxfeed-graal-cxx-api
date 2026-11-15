// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The warnings of a subscription about slow listeners and lagging events (the page listener_threads). The properties
// are read when a subscription is created, so every test case sets them before it creates its subscription.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;
using dxfcpp::test::ErrorManagerGuard;
using dxfcpp::test::LocalHubFixture;

namespace {

constexpr auto SLOW = "the listeners took";
constexpr auto LAG = "the newest event";

// Sets the properties of the checks for a test case and clears them at the end.
struct ListenerChecksProperties {
    ListenerChecksProperties(const std::string &slowListenerThreshold, const std::string &eventLagThreshold,
                             const std::string &warningPeriod) {
        set(slowListenerThreshold, eventLagThreshold, warningPeriod);
    }

    ~ListenerChecksProperties() {
        set("", "", "");
    }

    ListenerChecksProperties(const ListenerChecksProperties &) = delete;
    ListenerChecksProperties &operator=(const ListenerChecksProperties &) = delete;

  private:
    static void set(const std::string &slowListenerThreshold, const std::string &eventLagThreshold,
                    const std::string &warningPeriod) {
        REQUIRE(System::setProperty(DXFeedSubscription::SLOW_LISTENER_THRESHOLD_PROPERTY, slowListenerThreshold));
        REQUIRE(System::setProperty(DXFeedSubscription::EVENT_LAG_THRESHOLD_PROPERTY, eventLagThreshold));
        REQUIRE(System::setProperty(DXFeedSubscription::WARNING_PERIOD_PROPERTY, warningPeriod));
    }
};

bool contains(const std::string &string, const std::string &part) {
    return string.find(part) != std::string::npos;
}

// The warnings of the subscriptions whose message contains the part.
std::vector<ErrorInfo> warnings(const std::string &part) {
    std::vector<ErrorInfo> result{};

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.level == ErrorInfo::Level::WARN && error.component == "DXFeedSubscription" &&
            contains(error.message, part)) {
            result.push_back(error);
        }
    }

    return result;
}

// The number after the part of the message, for example the largest time after "the largest ".
std::int64_t numberAfter(const std::string &message, const std::string &part) {
    const auto position = message.find(part);

    REQUIRE(position != std::string::npos);

    return std::stoll(message.substr(position + part.size()));
}

std::int64_t nowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::shared_ptr<Quote> quote(double bidPrice, std::int64_t time = 0) {
    auto result = std::make_shared<Quote>("SLW");

    // A new price for every quote: the same quote again might not be delivered.
    result->setBidPrice(bidPrice);
    result->setAskPrice(bidPrice + 1.0);
    result->setBidTime(time);
    result->setAskTime(time);

    return result;
}

// A subscription to the quotes of SLW whose listener sleeps for the time in sleepFor before it returns.
struct SleepingListener {
    std::shared_ptr<DXFeedSubscription> subscription;
    std::chrono::milliseconds sleepFor{};

    explicit SleepingListener(const LocalHubFixture &hub)
        : subscription(hub.feed->createSubscription(Quote::TYPE)) {
        subscription->addEventListener([this](const auto &) {
            std::this_thread::sleep_for(sleepFor);
        });
        subscription->addSymbols("SLW");
    }

    ~SleepingListener() {
        subscription->close();
    }
};

} // namespace

TEST_CASE("Slow listener: one warning per period with the subscription and the time") {
    ErrorManagerGuard errorManagerGuard{};
    ListenerChecksProperties properties{"0.02s", "", "1m"};
    LocalHubFixture hub{};
    SleepingListener listener{hub};

    listener.sleepFor = 100ms;

    for (int i = 0; i < 3; i++) {
        hub.publishAndProcess(quote(10.0 + i));
    }

    const auto found = warnings(SLOW);

    REQUIRE(found.size() == 1);
    CHECK(contains(found[0].message, "DXFeedSubscription #"));
    CHECK(contains(found[0].message, "[Quote]"));
    CHECK(contains(found[0].message, "for a batch of 1 events"));
    CHECK(contains(found[0].message, "the threshold is 20 ms"));
    CHECK(contains(found[0].message, DXFeedSubscription::SLOW_LISTENER_THRESHOLD_PROPERTY));
    // A margin: the clock of sleep_for may differ from steady_clock by a millisecond.
    CHECK(numberAfter(found[0].message, SLOW + " "s) >= 90);
    CHECK_FALSE(contains(found[0].message, "Since the previous warning"));
}

TEST_CASE("Slow listener: the next warning after the period counts the suppressed batches") {
    ErrorManagerGuard errorManagerGuard{};
    ListenerChecksProperties properties{"0.02s", "", "1s"};
    LocalHubFixture hub{};
    SleepingListener listener{hub};

    listener.sleepFor = 50ms;
    hub.publishAndProcess(quote(20.0));

    // Suppressed: within the period after the first warning (about 0.2 s of a period of 1 s).
    listener.sleepFor = 60ms;
    hub.publishAndProcess(quote(21.0));
    listener.sleepFor = 150ms;
    hub.publishAndProcess(quote(22.0));

    REQUIRE(warnings(SLOW).size() == 1);

    std::this_thread::sleep_for(1200ms);
    listener.sleepFor = 50ms;
    hub.publishAndProcess(quote(23.0));

    auto found = warnings(SLOW);

    REQUIRE(found.size() == 2);
    CHECK(contains(found[1].message, "Since the previous warning: 2 more slow batches"));
    CHECK(numberAfter(found[1].message, "the largest ") >= 140);

    // The next period starts from zero.
    std::this_thread::sleep_for(1200ms);
    hub.publishAndProcess(quote(24.0));

    found = warnings(SLOW);

    REQUIRE(found.size() == 3);
    CHECK_FALSE(contains(found[2].message, "Since the previous warning"));
}

TEST_CASE("Slow listener: nothing is reported when the check is off or the listener is fast") {
    ErrorManagerGuard errorManagerGuard{};

    SUBCASE("The threshold 0 turns the check off") {
        ListenerChecksProperties properties{"0", "", ""};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        listener.sleepFor = 150ms;

        for (int i = 0; i < 3; i++) {
            hub.publishAndProcess(quote(30.0 + i));
        }
    }

    SUBCASE("The threshold inf is never reached") {
        ListenerChecksProperties properties{"inf", "", "inf"};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        listener.sleepFor = 150ms;
        hub.publishAndProcess(quote(40.0));
    }

    SUBCASE("A fast listener with the default threshold") {
        ListenerChecksProperties properties{"", "", ""};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        for (int i = 0; i < 3; i++) {
            hub.publishAndProcess(quote(50.0 + i));
        }
    }

    CHECK(warnings(SLOW).empty());
}

TEST_CASE("Event lag: an old event is reported, a fresh one, one without a time or with the check off are not") {
    ErrorManagerGuard errorManagerGuard{};

    SUBCASE("Old quote") {
        ListenerChecksProperties properties{"0", "5s", "1m"};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        hub.publishAndProcess(quote(60.0, nowMillis() - 60'000));
        hub.publishAndProcess(quote(61.0, nowMillis() - 60'000));

        const auto found = warnings(LAG);

        REQUIRE(found.size() == 1);
        CHECK(contains(found[0].message, "[Quote]"));
        CHECK(contains(found[0].message, "the threshold is 5000 ms"));
        CHECK(contains(found[0].message, DXFeedSubscription::EVENT_LAG_THRESHOLD_PROPERTY));
        CHECK(numberAfter(found[0].message, "events is ") >= 60'000);
    }

    SUBCASE("Old time and sale") {
        ListenerChecksProperties properties{"0", "5s", "1m"};
        LocalHubFixture hub{};
        const auto subscription = hub.feed->createSubscription(TimeAndSale::TYPE);
        std::size_t received{};

        subscription->addEventListener([&received](const auto &events) {
            received += events.size();
        });
        subscription->addSymbols("SLW");

        auto timeAndSale = std::make_shared<TimeAndSale>("SLW");

        timeAndSale->setTime(nowMillis() - 60'000);
        timeAndSale->setPrice(70.0);
        hub.publishAndProcess(timeAndSale);

        REQUIRE(received == 1);
        CHECK(warnings(LAG).size() == 1);
        subscription->close();
    }

    SUBCASE("Old trade and trade ETH") {
        ListenerChecksProperties properties{"0", "5s", "1m"};
        LocalHubFixture hub{};
        const auto trades = hub.feed->createSubscription(Trade::TYPE);
        const auto tradesEth = hub.feed->createSubscription(TradeETH::TYPE);
        std::size_t received{};
        const auto count = [&received](const auto &events) {
            received += events.size();
        };

        trades->addEventListener(count);
        trades->addSymbols("SLW");
        tradesEth->addEventListener(count);
        tradesEth->addSymbols("SLW");

        auto trade = std::make_shared<Trade>("SLW");
        auto tradeEth = std::make_shared<TradeETH>("SLW");

        trade->setTime(nowMillis() - 60'000);
        trade->setPrice(71.0);
        tradeEth->setTime(nowMillis() - 60'000);
        tradeEth->setPrice(72.0);
        hub.publishAndProcess(trade);
        hub.publishAndProcess(tradeEth);

        const auto found = warnings(LAG);

        REQUIRE(received == 2);
        REQUIRE(found.size() == 2);
        CHECK(contains(found[0].message + found[1].message, "[Trade]"));
        CHECK(contains(found[0].message + found[1].message, "[TradeETH]"));
        trades->close();
        tradesEth->close();
    }

    SUBCASE("Fresh quote and quote without a time") {
        ListenerChecksProperties properties{"0", "5s", "1m"};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        hub.publishAndProcess(quote(80.0, nowMillis()));
        hub.publishAndProcess(quote(81.0));

        CHECK(warnings(LAG).empty());
    }

    SUBCASE("The check is off by default") {
        ListenerChecksProperties properties{"0", "", ""};
        LocalHubFixture hub{};
        SleepingListener listener{hub};

        hub.publishAndProcess(quote(90.0, nowMillis() - 60'000));

        CHECK(warnings(LAG).empty());
    }
}

TEST_CASE("Event lag: a time series subscription is not checked") {
    ErrorManagerGuard errorManagerGuard{};
    ListenerChecksProperties properties{"0", "5s", "1m"};
    LocalHubFixture hub{};
    const auto subscription = hub.feed->createTimeSeriesSubscription(TimeAndSale::TYPE);
    std::size_t received{};

    subscription->addEventListener([&received](const auto &events) {
        received += events.size();
    });
    subscription->setFromTime(0);
    subscription->addSymbols("SLW");

    auto timeAndSale = std::make_shared<TimeAndSale>("SLW");

    timeAndSale->setTime(nowMillis() - 60'000);
    timeAndSale->setPrice(100.0);
    hub.publishAndProcess(timeAndSale);

    REQUIRE(received == 1);
    CHECK(warnings(LAG).empty());
    subscription->close();
}

TEST_CASE("A property that is not a time period: one warning, the default is used") {
    ErrorManagerGuard errorManagerGuard{};
    ListenerChecksProperties properties{"slow", "", ""};
    LocalHubFixture hub{};
    SleepingListener first{hub};
    SleepingListener second{hub};

    const auto invalid = warnings("is not a time period");

    REQUIRE(invalid.size() == 1);
    CHECK(contains(invalid[0].message, "'slow'"));
    CHECK(contains(invalid[0].message, DXFeedSubscription::SLOW_LISTENER_THRESHOLD_PROPERTY));
    CHECK(contains(invalid[0].message, "the default 0.1s is used"));

    // The default threshold of 0.1 s works.
    first.sleepFor = 150ms;
    hub.publishAndProcess(quote(110.0));

    REQUIRE(warnings(SLOW).size() >= 1);
    CHECK(contains(warnings(SLOW)[0].message, "the threshold is 100 ms"));
}
