// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;
using namespace std::chrono_literals;

namespace {

using Events = std::vector<std::shared_ptr<EventType>>;

auto subscriptionManager() {
    return ApiContext::getInstance()->getManager<WeakEntityManager<DXFeedSubscription>>();
}

auto endpointManager() {
    return ApiContext::getInstance()->getManager<WeakEntityManager<DXEndpoint>>();
}

std::shared_ptr<DXEndpoint> localHub() {
    return DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::LOCAL_HUB)->build();
}

std::shared_ptr<Quote> quote(double bidPrice) {
    auto event = std::make_shared<Quote>("AAPL");

    event->setBidPrice(bidPrice);

    return event;
}

// Runs the function on another thread and returns false if it does not return in time (a deadlock).
template <typename F> bool returnsInTime(F &&function, std::chrono::milliseconds timeout = 10s) {
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();

    std::thread{[done, function = std::forward<F>(function)]() mutable {
        function();
        done->set_value();
    }}.detach();

    return future.wait_for(timeout) == std::future_status::ready;
}

} // namespace

// The header template DXFeedSubscription::create(EventTypesCollection&&) is compiled into this test, while
// DXFeedSubscription::create(const EventTypeEnum &) is compiled into the library. With the library linked dynamically,
// both register in one manager and must not get the same id.
TEST_CASE("Subscriptions registered from the test and from the library get distinct ids") {
    constexpr int count = 50;
    std::vector<std::shared_ptr<DXFeedSubscription>> subscriptions{};
    std::set<std::size_t> ids{};

    for (int i = 0; i < count; ++i) {
        subscriptions.push_back(DXFeedSubscription::create(std::vector{Quote::TYPE}));
        subscriptions.push_back(DXFeedSubscription::create(Quote::TYPE));
    }

    for (const auto &subscription : subscriptions) {
        const auto id = subscriptionManager()->getId(subscription);

        REQUIRE(id.has_value());
        ids.insert(id->getValue());
    }

    CHECK(ids.size() == subscriptions.size());

    for (const auto &subscription : subscriptions) {
        subscription->close();
    }
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "Closed subscriptions do not stay in the registry") {
    const auto initialSize = subscriptionManager()->size();

    for (int i = 0; i < 10'000; ++i) {
        const auto subscription = feed->createSubscription(Quote::TYPE);

        subscription->addEventListener([](const Events &) {
        });
        subscription->addSymbols("AAPL");
        subscription->close();
    }

    CHECK(subscriptionManager()->size() == initialSize);
}

TEST_CASE("A subscription without a feed is closed and released with its last pointer") {
    const auto initialSize = subscriptionManager()->size();
    auto subscription = DXFeedSubscription::create(Quote::TYPE);
    const std::weak_ptr<DXFeedSubscription> weak = subscription;

    subscription->addSymbols("AAPL");
    subscription.reset();

    CHECK(weak.expired());
    CHECK(subscriptionManager()->size() == initialSize);
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "An attached subscription without a pointer of the user receives events") {
    std::atomic<int> calls{0};
    std::weak_ptr<DXFeedSubscription> weak{};

    {
        const auto subscription = feed->createSubscription(Quote::TYPE);

        subscription->addEventListener([&](const Events &) {
            calls++;
        });
        subscription->addSymbols("AAPL");
        weak = subscription;
    }

    publishAndProcess(quote(10.5));

    CHECK_FALSE(weak.expired());
    CHECK(calls.load() == 1);

    weak.lock()->close();
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "A detached subscription is released with the last pointer of the user") {
    auto subscription = feed->createSubscription(Quote::TYPE);
    const std::weak_ptr<DXFeedSubscription> weak = subscription;

    feed->detachSubscription(subscription);
    subscription.reset();

    CHECK(weak.expired());
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "A closed subscription is released by its feed") {
    auto subscription = feed->createSubscription(Quote::TYPE);
    const std::weak_ptr<DXFeedSubscription> weak = subscription;

    subscription->addSymbols("AAPL");
    subscription->close();
    subscription.reset();

    CHECK(weak.expired());
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "The listeners of a subscription are not called after close() returns") {
    const auto subscription = feed->createSubscription(Quote::TYPE);
    std::atomic<int> calls{0};
    std::atomic<bool> stop{false};

    subscription->addEventListener([&](const Events &) {
        calls++;
        std::this_thread::sleep_for(1ms);
    });
    subscription->addSymbols("AAPL");

    std::thread notifier{[&] {
        for (int i = 0; !stop; ++i) {
            publishAndProcess(quote(i));
        }
    }};

    REQUIRE(test::waitUntil([&] {
        return calls.load() > 5;
    }));

    subscription->close();

    const auto callsAfterClose = calls.load();

    std::this_thread::sleep_for(100ms);
    stop = true;
    notifier.join();

    CHECK(calls.load() == callsAfterClose);
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "A subscription can be closed from its listener") {
    const auto subscription = feed->createSubscription(Quote::TYPE);
    std::atomic<int> calls{0};

    subscription->addEventListener([&](const Events &) {
        calls++;
        subscription->close();
    });
    subscription->addSymbols("AAPL");

    REQUIRE(returnsInTime([this] {
        publishAndProcess(quote(10.5));
        publishAndProcess(quote(11.5));
    }));

    CHECK(calls.load() == 1);
    CHECK(subscription->isClosed());
}

// Each listener closes the subscription of the other while the other is called on another thread: a wait for the
// call in progress in close() would deadlock.
TEST_CASE("Two listeners can close the subscriptions of each other on two threads") {
    test::LocalHubFixture first{};
    test::LocalHubFixture second{};
    const auto firstSubscription = first.feed->createSubscription(Quote::TYPE);
    const auto secondSubscription = second.feed->createSubscription(Quote::TYPE);
    std::latch bothCalled{2};

    firstSubscription->addEventListener([&](const Events &) {
        bothCalled.arrive_and_wait();
        secondSubscription->close();
    });
    secondSubscription->addEventListener([&](const Events &) {
        bothCalled.arrive_and_wait();
        firstSubscription->close();
    });
    firstSubscription->addSymbols("AAPL");
    secondSubscription->addSymbols("AAPL");

    REQUIRE(returnsInTime([&] {
        std::thread firstNotifier{[&] {
            first.publishAndProcess(quote(10.5));
        }};
        std::thread secondNotifier{[&] {
            second.publishAndProcess(quote(10.5));
        }};

        firstNotifier.join();
        secondNotifier.join();
    }));

    CHECK(firstSubscription->isClosed());
    CHECK(secondSubscription->isClosed());
}

TEST_CASE_FIXTURE(test::LocalHubFixture,
                  "A listener can change the change listeners while the subscription is closed on another thread") {
    const auto subscription = feed->createSubscription(Quote::TYPE);
    std::promise<void> entered{};
    std::atomic<bool> enteredOnce{false};
    std::atomic<bool> stop{false};

    subscription->addEventListener([&](const Events &) {
        if (!enteredOnce.exchange(true)) {
            entered.set_value();
        }

        const auto id = subscription->addChangeListener(
            ObservableSubscriptionChangeListener::create([](const std::unordered_set<SymbolWrapper> &) {
            }));

        subscription->removeChangeListener(id);
    });
    subscription->addSymbols("AAPL");

    std::thread notifier{[&] {
        for (int i = 0; !stop; ++i) {
            publishAndProcess(quote(i));
        }
    }};

    REQUIRE(entered.get_future().wait_for(10s) == std::future_status::ready);

    const bool closed = returnsInTime([subscription] {
        subscription->close();
    });

    stop = true;
    notifier.join();

    CHECK(closed);
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "The methods of a closed subscription do nothing") {
    const auto subscription = feed->createSubscription(Quote::TYPE);
    std::atomic<int> calls{0};

    subscription->close();

    CHECK_NOTHROW(subscription->addEventListener([&](const Events &) {
        calls++;
    }));
    CHECK_NOTHROW(subscription->addSymbols("AAPL"));
    CHECK_NOTHROW(feed->attachSubscription(subscription));

    publishAndProcess(quote(10.5));

    CHECK(calls.load() == 0);
}

TEST_CASE("Closed endpoints are released and do not stay in the registry") {
    const auto initialSize = endpointManager()->size();
    std::vector<std::weak_ptr<DXEndpoint>> endpoints{};

    // A Java endpoint takes milliseconds to create: 500 are enough to see a leak.
    for (int i = 0; i < 500; ++i) {
        const auto endpoint = localHub();

        ignoreUnused(endpoint->getFeed());
        ignoreUnused(endpoint->getPublisher());
        endpoint->close();

        if (i % 50 == 0) {
            endpoints.push_back(endpoint);
        }
    }

    // The CLOSED notification may come after close() returns and keep the endpoint for a while.
    CHECK(test::waitUntil([&] {
        return std::ranges::all_of(endpoints, [](const auto &endpoint) {
            return endpoint.expired();
        });
    }));
    CHECK(test::waitUntil([&] {
        return endpointManager()->size() == initialSize;
    }));
}

TEST_CASE("An endpoint closed by closeAndAwaitTermination() is released") {
    auto endpoint = localHub();
    const std::weak_ptr<DXEndpoint> weak = endpoint;

    ignoreUnused(endpoint->getFeed());
    endpoint->closeAndAwaitTermination();
    endpoint.reset();

    CHECK(test::waitUntil([&] {
        return weak.expired();
    }));
}

TEST_CASE("A created endpoint without a pointer of the user lives until it is closed") {
    std::weak_ptr<DXEndpoint> weak = localHub();

    CHECK_FALSE(weak.expired());

    weak.lock()->close();

    CHECK(test::waitUntil([&] {
        return weak.expired();
    }));
}

TEST_CASE("A default endpoint stays the default one after it is closed") {
    const auto endpoint = DXEndpoint::getInstance(DXEndpoint::Role::LOCAL_HUB);

    endpoint->close();

    CHECK(DXEndpoint::getInstance(DXEndpoint::Role::LOCAL_HUB) == endpoint);
}

TEST_CASE("The methods of a closed endpoint do nothing") {
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();
    std::atomic<int> calls{0};

    endpoint->close();

    REQUIRE(test::waitUntil([&] {
        return endpoint->isClosed();
    }));
    CHECK_NOTHROW(endpoint->addStateChangeListener([&](DXEndpoint::State, DXEndpoint::State) {
        calls++;
    }));
    CHECK_NOTHROW(endpoint->connect("127.0.0.1:1"));
    CHECK_NOTHROW(endpoint->reconnect());
    CHECK_NOTHROW(endpoint->disconnect());

    std::this_thread::sleep_for(100ms);

    CHECK(calls.load() == 0);
    CHECK(endpoint->getState() == DXEndpoint::State::CLOSED);
}

TEST_CASE("CLOSED is the last state notification of an endpoint") {
    const auto endpoint = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::FEED)->build();
    std::mutex mutex{};
    std::vector<DXEndpoint::State> states{};

    endpoint->addStateChangeListener([&](DXEndpoint::State, DXEndpoint::State newState) {
        std::lock_guard guard{mutex};

        states.push_back(newState);
    });

    test::LoopbackTcpServer server{};

    endpoint->connect(server.getAddress());
    endpoint->close();
    endpoint->reconnect();

    REQUIRE(test::waitUntil([&] {
        std::lock_guard guard{mutex};

        return !states.empty() && states.back() == DXEndpoint::State::CLOSED;
    }));

    std::this_thread::sleep_for(100ms);

    std::lock_guard guard{mutex};

    CHECK(std::ranges::count(states, DXEndpoint::State::CLOSED) == 1);
    CHECK(states.back() == DXEndpoint::State::CLOSED);
}

TEST_CASE("An attached subscription without a pointer of the user is released when its endpoint is closed") {
    test::LocalHubFixture hub{};
    std::atomic<int> calls{0};
    std::weak_ptr<DXFeedSubscription> weak{};

    {
        const auto subscription = hub.feed->createSubscription(Quote::TYPE);

        subscription->addEventListener([&](const Events &) {
            calls++;
        });
        subscription->addSymbols("AAPL");
        weak = subscription;
    }

    hub.publishAndProcess(quote(10.5));
    REQUIRE(calls.load() == 1);

    hub.endpoint->close();
    hub.publishAndProcess(quote(11.5));

    CHECK(calls.load() == 1);
    CHECK(test::waitUntil([&] {
        return weak.expired();
    }));
}

TEST_CASE("A subscription of the user stays valid after its endpoint is closed") {
    test::LocalHubFixture hub{};
    const auto subscription = hub.feed->createSubscription(Quote::TYPE);

    hub.endpoint->close();

    CHECK_NOTHROW(subscription->addSymbols("AAPL"));
    CHECK_NOTHROW(subscription->getSymbols());
    CHECK_NOTHROW(subscription->close());
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "An endpoint can be closed from a listener of its subscription") {
    const auto subscription = feed->createSubscription(Quote::TYPE);
    std::atomic<int> calls{0};

    subscription->addEventListener([&](const Events &) {
        calls++;
        endpoint->close();
    });
    subscription->addSymbols("AAPL");

    REQUIRE(returnsInTime([this] {
        publishAndProcess(quote(10.5));
        publishAndProcess(quote(11.5));
    }));

    CHECK(calls.load() == 1);
    CHECK(endpoint->isClosed());
}

TEST_CASE("The subscriptions of a publisher are released when its endpoint is closed") {
    test::LocalHubFixture hub{};
    std::weak_ptr<ObservableSubscription> weak{};

    {
        const auto subscription = hub.publisher->getSubscription(Quote::TYPE);

        CHECK(hub.publisher->getSubscription(Quote::TYPE) == subscription);
        weak = subscription;
    }

    REQUIRE_FALSE(weak.expired());

    hub.endpoint->close();

    CHECK(test::waitUntil([&] {
        return weak.expired();
    }));
}

TEST_CASE("executor() called after getFeed() applies to the next events of the existing subscriptions") {
    const auto endpoint = localHub();
    const auto feed = endpoint->getFeed();
    const auto publisher = endpoint->getPublisher();
    const auto subscription = feed->createSubscription(Quote::TYPE);
    const auto executor = InPlaceExecutor::create();
    std::atomic<int> calls{0};
    std::atomic<std::thread::id> listenerThread{};

    subscription->addEventListener([&](const Events &) {
        listenerThread = std::this_thread::get_id();
        calls++;
    });
    subscription->addSymbols("AAPL");
    endpoint->executor(executor);
    publisher->publishEvents(quote(10.5));

    REQUIRE(test::waitUntil([&] {
        executor->processAllPendingTasks();

        return calls.load() > 0;
    }));

    CHECK(listenerThread.load() == std::this_thread::get_id());

    endpoint->close();
}
