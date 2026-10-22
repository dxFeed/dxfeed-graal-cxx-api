// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

// The model notifies its listener through the endpoint executor (see process()) unless it has its own executor.
namespace {

constexpr auto SYMBOL = "TEST-SYMBOL";

constexpr std::int32_t TX_PENDING = 0x01;
constexpr std::int32_t SNAPSHOT_BEGIN = 0x04;
constexpr std::int32_t SNAPSHOT_END = 0x08;

struct Notification {
    std::vector<std::shared_ptr<Order>> events;
    bool isSnapshot;
};

struct IndexedEventTxModelFixture : test::LocalHubFixture {
    std::mutex mutex{};
    std::vector<Notification> notifications{};
    std::shared_ptr<IndexedEventTxModel<Order>> model{};

    std::shared_ptr<IndexedEventTxModel<Order>::Builder> builder(bool withFeed = true) {
        auto b = IndexedEventTxModel<Order>::newBuilder()->withSymbol(SYMBOL)->withSource(OrderSource::DEFAULT);

        if (withFeed) {
            b->withFeed(feed);
        }

        return b->withListener([this](const std::vector<std::shared_ptr<Order>> &events, bool isSnapshot) {
            std::lock_guard lock{mutex};

            notifications.push_back({events, isSnapshot});
        });
    }

    void publish(std::int64_t index, double size, std::int32_t eventFlags,
                 const OrderSource &source = OrderSource::DEFAULT) const {
        auto order = std::make_shared<Order>(SYMBOL);

        order->setIndex(index);
        order->setSource(source);
        order->setSize(size);
        order->setPrice(100.0 + static_cast<double>(index));
        order->setEventFlags(eventFlags);
        order->setOrderSide(Side::BUY);

        publisher->publishEvents(order);
    }

    // Processes the pending tasks of the endpoint executor and returns the notifications received since the last call.
    std::vector<Notification> takeNotifications() {
        process();

        std::lock_guard lock{mutex};

        return std::exchange(notifications, {});
    }

    // The last event of a snapshot of orders has the index 0.
    void publishSnapshot() {
        publish(2, 20, SNAPSHOT_BEGIN);
        publish(1, 10, 0);
        publish(0, 0.5, SNAPSHOT_END);
    }

    static std::int32_t sourceId(const EventSourceWrapper &source) {
        return source.isOrderSource() ? source.asOrderSource()->id() : source.asIndexedEventSource()->id();
    }

    static void requireEvents(const Notification &notification, const std::vector<std::int64_t> &indices) {
        REQUIRE(notification.events.size() == indices.size());

        for (std::size_t i = 0; i < indices.size(); i++) {
            INFO("order: ", notification.events[i]->toString());
            REQUIRE(notification.events[i]->getEventSymbol() == SYMBOL);
            REQUIRE(notification.events[i]->getIndex() == indices[i]);
        }
    }
};

} // namespace

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: the symbol and the source") {
    model = builder()->withSource(OrderSource::NTV)->build();

    REQUIRE(model->getSymbol() == SYMBOL);
    REQUIRE(sourceId(model->getSource()) == OrderSource::NTV.id());

    const auto symbol = model->getIndexedEventSubscriptionSymbol();

    REQUIRE(symbol.getEventSymbol()->toStringUnderlying() == SYMBOL);
    REQUIRE(symbol.getSource()->id() == OrderSource::NTV.id());

    auto model2 = IndexedEventTxModel<Order>::newBuilder()
                      ->withSymbol(IndexedEventSubscriptionSymbol(SYMBOL, OrderSource::DEX))
                      ->withListener([](const auto &, bool) {
                      })
                      ->build();

    REQUIRE(model2->getSymbol() == SYMBOL);
    REQUIRE(sourceId(model2->getSource()) == OrderSource::DEX.id());
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: the source and the listener are required") {
    REQUIRE_THROWS(IndexedEventTxModel<Order>::newBuilder()->withSymbol(SYMBOL)->withSource(OrderSource::NTV)->build());
    REQUIRE_THROWS(IndexedEventTxModel<Order>::newBuilder()
                       ->withSymbol(SYMBOL)
                       ->withListener([](const auto &, bool) {
                       })
                       ->build());
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: invalid arguments") {
    REQUIRE_THROWS_AS(IndexedEventTxModel<Order>::newBuilder()->withListener(nullptr), InvalidArgumentException);
    REQUIRE_THROWS_AS(IndexedEventTxModel<Order>::newBuilder()->withFeed(nullptr), InvalidArgumentException);

    model = builder()->build();

    const auto controller = model->getSubscriptionController();

    REQUIRE_THROWS_AS(controller->attach(nullptr), InvalidArgumentException);
    REQUIRE_THROWS_AS(controller->detach(nullptr), InvalidArgumentException);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: snapshot and transactions") {
    model = builder()->build();

    REQUIRE(takeNotifications().empty());

    publishSnapshot();

    auto received = takeNotifications();

    REQUIRE(received.size() == 1);
    REQUIRE(received[0].isSnapshot);
    requireEvents(received[0], {2, 1, 0});

    // A transaction is delivered when it ends.
    publish(1, 11, TX_PENDING);
    publish(2, 21, TX_PENDING);
    REQUIRE(takeNotifications().empty());

    publish(3, 30, 0);
    received = takeNotifications();

    REQUIRE(received.size() == 1);
    REQUIRE_FALSE(received[0].isSnapshot);
    requireEvents(received[0], {1, 2, 3});
    REQUIRE(received[0].events[0]->getSize() == 11);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: only the events of the source") {
    model = builder()->withSource(OrderSource::NTV)->build();

    publish(0, 10, SNAPSHOT_BEGIN | SNAPSHOT_END, OrderSource::DEX);
    REQUIRE(takeNotifications().empty());

    publish(0, 10, SNAPSHOT_BEGIN | SNAPSHOT_END, OrderSource::NTV);

    const auto received = takeNotifications();

    REQUIRE(received.size() == 1);
    REQUIRE(received[0].events.size() == 1);
    REQUIRE(received[0].events[0]->getSource() == OrderSource::NTV);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: the settings of the subscription controller") {
    model = builder()->build();

    // The aggregation period that is not set reads as zero.
    REQUIRE(model->getSubscriptionController()->getAggregationPeriod().getTime() == 0);

    model = builder()->withEventsBatchLimit(5)->withAggregationPeriod(50ms)->build();

    const auto controller = model->getSubscriptionController();

    REQUIRE(controller->getEventsBatchLimit() == 5);
    REQUIRE(controller->getAggregationPeriod().getTime() == 50);

    // A copy: REQUIRE binds its operands by reference, and the static member has no out-of-class definition.
    constexpr std::int32_t optimalBatchLimit = DXFeedSubscription::OPTIMAL_BATCH_LIMIT;

    controller->setEventsBatchLimit(optimalBatchLimit);
    controller->setAggregationPeriod(TimePeriod::valueOf(0));

    REQUIRE(controller->getEventsBatchLimit() == optimalBatchLimit);
    REQUIRE(controller->getAggregationPeriod().getTime() == 0);

    controller->setAggregationPeriod(std::int64_t{100});

    // Each call returns a new controller of the same subscription.
    REQUIRE(model->getSubscriptionController()->getAggregationPeriod().getTime() == 100);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: attach and detach the feed") {
    model = builder(false)->build();

    publish(0, 10, SNAPSHOT_BEGIN | SNAPSHOT_END);
    REQUIRE(takeNotifications().empty()); // the model is not attached to the feed

    const auto controller = model->getSubscriptionController();

    controller->attach(feed);
    publish(0, 11, SNAPSHOT_BEGIN | SNAPSHOT_END);
    REQUIRE(takeNotifications().size() == 1);

    controller->detach(feed);
    publish(0, 12, SNAPSHOT_BEGIN | SNAPSHOT_END);
    REQUIRE(takeNotifications().empty()); // the model is detached

    controller->attach(feed);
    publish(0, 13, SNAPSHOT_BEGIN | SNAPSHOT_END);

    const auto received = takeNotifications();

    REQUIRE(received.size() == 1);
    REQUIRE(received[0].isSnapshot);
    requireEvents(received[0], {0});
    REQUIRE(received[0].events[0]->getSize() == 13);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: the executor") {
    const auto modelExecutor = InPlaceExecutor::create();

    model = builder()->withExecutor(modelExecutor)->build();

    publishSnapshot();
    REQUIRE(takeNotifications().empty()); // the endpoint executor does not notify the listener

    modelExecutor->processAllPendingTasks();
    REQUIRE(takeNotifications().size() == 1);

    // Back to the endpoint executor.
    model->getSubscriptionController()->setExecutor(nullptr);
    publish(4, 40, 0);
    REQUIRE(takeNotifications().size() == 1);

    model->getSubscriptionController()->setExecutor(modelExecutor);
    publish(5, 50, 0);
    REQUIRE(takeNotifications().empty());

    modelExecutor->processAllPendingTasks();
    REQUIRE(takeNotifications().size() == 1);
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: an exception of the listener is not propagated") {
    std::vector<std::size_t> sizes{};

    model = IndexedEventTxModel<Order>::newBuilder()
                ->withFeed(feed)
                ->withSymbol(SYMBOL)
                ->withSource(OrderSource::DEFAULT)
                ->withListener([&sizes](const std::vector<std::shared_ptr<Order>> &events, bool) {
                    sizes.push_back(events.size());

                    throw std::runtime_error("listener failure");
                })
                ->build();

    publishSnapshot();
    REQUIRE_NOTHROW(process());

    publish(4, 40, 0);
    REQUIRE_NOTHROW(process());

    REQUIRE(sizes == std::vector<std::size_t>{3, 1});
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: models built by one builder") {
    const auto b = builder();
    auto model1 = b->build();
    const auto model2 = b->build();

    // Both models share the listener of the builder: closing one does not stop the other.
    model1.reset();

    publishSnapshot();

    const auto received = takeNotifications();

    REQUIRE(received.size() == 1);
    requireEvents(received[0], {2, 1, 0});
    REQUIRE_FALSE(model2->isClosed());
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: no notifications after close") {
    model = builder()->build();

    REQUIRE_FALSE(model->isClosed());

    model->close();
    model->close();

    REQUIRE(model->isClosed());

    publishSnapshot();
    REQUIRE(takeNotifications().empty());
}

TEST_CASE_FIXTURE(IndexedEventTxModelFixture, "IndexedEventTxModel: the model is destroyed by its listener") {
    std::vector<std::size_t> sizes{};

    model = IndexedEventTxModel<Order>::newBuilder()
                ->withFeed(feed)
                ->withSymbol(SYMBOL)
                ->withSource(OrderSource::DEFAULT)
                ->withListener([this, &sizes](const std::vector<std::shared_ptr<Order>> &events, bool) {
                    model.reset(); // closes the model and releases the listener during this call

                    sizes.push_back(events.size());
                })
                ->build();

    publishSnapshot();
    process();

    REQUIRE(model == nullptr);
    REQUIRE(sizes == std::vector<std::size_t>{3});

    publish(4, 40, 0);
    process();

    REQUIRE(sizes.size() == 1);
}
