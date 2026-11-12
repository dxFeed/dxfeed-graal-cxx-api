// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

using namespace dxfcpp;
using namespace std::literals;

namespace {

std::shared_ptr<ObservableSubscriptionChangeListener> createNoOpChangeListener() {
    return ObservableSubscriptionChangeListener::create(
        [](const std::unordered_set<SymbolWrapper> &) {},
        [](const std::unordered_set<SymbolWrapper> &) {}, [] {});
}

} // namespace

TEST_CASE("An unregistered observable-subscription listener is released with its last owner") {
    std::weak_ptr<ObservableSubscriptionChangeListener> weakListener;

    {
        auto listener = createNoOpChangeListener();
        weakListener = listener;
    }

    CHECK(weakListener.expired());
}

TEST_CASE("A feed subscription owns a change listener only while it is registered") {
    auto subscription = DXFeedSubscription::create(Quote::TYPE);
    auto listener = createNoOpChangeListener();
    std::weak_ptr<ObservableSubscriptionChangeListener> weakListener = listener;
    const auto listenerId = subscription->addChangeListener(listener);

    listener.reset();
    REQUIRE_FALSE(weakListener.expired());

    subscription->removeChangeListener(listenerId);
    CHECK(weakListener.expired());
}

TEST_CASE("A feed subscription releases its change listeners when closed") {
    auto subscription = DXFeedSubscription::create(Quote::TYPE);
    auto listener = createNoOpChangeListener();
    std::weak_ptr<ObservableSubscriptionChangeListener> weakListener = listener;

    subscription->addChangeListener(listener);
    listener.reset();
    REQUIRE_FALSE(weakListener.expired());

    subscription->close();
    CHECK(weakListener.expired());
}

TEST_CASE("A listener shared by subscriptions lives until its final registration is removed") {
    auto first = DXFeedSubscription::create(Quote::TYPE);
    auto second = DXFeedSubscription::create(Trade::TYPE);
    auto listener = createNoOpChangeListener();
    std::weak_ptr<ObservableSubscriptionChangeListener> weakListener = listener;
    const auto firstId = first->addChangeListener(listener);
    const auto secondId = second->addChangeListener(listener);

    listener.reset();
    first->removeChangeListener(firstId);
    REQUIRE_FALSE(weakListener.expired());

    second->removeChangeListener(secondId);
    CHECK(weakListener.expired());
}

TEST_CASE("A publisher subscription releases its change listeners when the endpoint closes") {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto subscription = endpoint->getPublisher()->getSubscription(Quote::TYPE);
    std::size_t closeNotifications = 0;
    auto listener = ObservableSubscriptionChangeListener::create(
        [](const std::unordered_set<SymbolWrapper> &) {},
        [](const std::unordered_set<SymbolWrapper> &) {}, [&closeNotifications] { ++closeNotifications; });
    std::weak_ptr<ObservableSubscriptionChangeListener> weakListener = listener;

    subscription->addChangeListener(listener);
    listener.reset();
    REQUIRE_FALSE(weakListener.expired());

    endpoint->closeAndAwaitTermination();
    CHECK_EQ(closeNotifications, 1);
    CHECK(weakListener.expired());
}

TEST_CASE("A publisher subscription change listener gets added and removed symbols and none after its removal") {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto feedSubscription = endpoint->getFeed()->createSubscription(Quote::TYPE);
    auto subscription = endpoint->getPublisher()->getSubscription(Quote::TYPE);
    std::mutex mutex{};
    std::unordered_set<std::string> current{};
    std::size_t calls = 0;
    auto listener = ObservableSubscriptionChangeListener::create(
        [&](const std::unordered_set<SymbolWrapper> &symbols) {
            std::lock_guard lock{mutex};

            ++calls;

            for (const auto &symbol : symbols) {
                current.insert(symbol.toStringUnderlying());
            }
        },
        [&](const std::unordered_set<SymbolWrapper> &symbols) {
            std::lock_guard lock{mutex};

            ++calls;

            for (const auto &symbol : symbols) {
                current.erase(symbol.toStringUnderlying());
            }
        },
        [] {});
    const auto waitFor = [&](auto predicate) {
        for (int i = 0; i < 500; i++) {
            {
                std::lock_guard lock{mutex};

                if (predicate()) {
                    return true;
                }
            }

            std::this_thread::sleep_for(10ms);
        }

        return false;
    };

    const auto listenerId = subscription->addChangeListener(listener);

    feedSubscription->addSymbols("AAPL");
    CHECK(waitFor([&] {
        return current.count("AAPL") == 1;
    }));

    feedSubscription->removeSymbols("AAPL");
    CHECK(waitFor([&] {
        return current.empty();
    }));

    subscription->removeChangeListener(listenerId);

    std::size_t callsAfterRemoval{};

    {
        std::lock_guard lock{mutex};

        callsAfterRemoval = calls;
    }

    feedSubscription->addSymbols("MSFT");
    std::this_thread::sleep_for(200ms);

    {
        std::lock_guard lock{mutex};

        CHECK_EQ(calls, callsAfterRemoval);
    }

    endpoint->closeAndAwaitTermination();
}

TEST_CASE("An unused instrument-profile connection is closed and released with its last owner") {
    auto collector = InstrumentProfileCollector::create();
    std::weak_ptr<InstrumentProfileConnection> weakConnection;

    {
        auto connection = InstrumentProfileConnection::createConnection("file:unused-lifecycle-test.ipf", collector);
        weakConnection = connection;
    }

    CHECK(weakConnection.expired());
}
