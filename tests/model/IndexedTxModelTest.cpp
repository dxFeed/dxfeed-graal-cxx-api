// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

// The scenarios follow the Java API tests of IndexedTxModel, the ones that publish events through
// a LOCAL_HUB publisher. The model notifies its listener through the endpoint executor, see process().
namespace {

constexpr auto SYMBOL = "TEST-SYMBOL";

struct IndexedTxModelFixture : test::LocalHubFixture {
    std::mutex mutex{};
    int listenerNotifications{};
    int snapshotNotifications{};
    std::deque<std::shared_ptr<Order>> receivedEvents{};
    std::vector<std::shared_ptr<Order>> publishedEvents{};
    std::shared_ptr<IndexedTxModel<Order>> model{};

    ~IndexedTxModelFixture() {
        if (model) {
            model->close();
        }
    }

    std::shared_ptr<IndexedTxModel<Order>::Builder> builder(bool withFeed = true) {
        auto b = IndexedTxModel<Order>::newBuilder();

        if (withFeed) {
            b = b->withFeed(feed);
        }

        return b->withSymbol(SYMBOL)
            ->withListener([this](const IndexedEventSource &, const std::vector<std::shared_ptr<Order>> &events,
                                  bool isSnapshot) {
                std::lock_guard lock{mutex};

                ++listenerNotifications;

                if (isSnapshot) {
                    ++snapshotNotifications;
                }

                receivedEvents.insert(receivedEvents.end(), events.begin(), events.end());
            });
    }

    static std::shared_ptr<Order> createOrder(std::int64_t index, const OrderSource &source, double size,
                                              std::int32_t eventFlags) {
        auto order = std::make_shared<Order>(SYMBOL);

        order->setIndex(index);
        order->setSource(source);
        order->setSize(size);
        order->setEventFlags(eventFlags);
        order->setOrderSide(Side::BUY);

        return order;
    }

    void addToPublish(std::int64_t index, double size, std::int32_t eventFlags = 0) {
        publishedEvents.push_back(createOrder(index, OrderSource::DEFAULT, size, eventFlags));
    }

    void publishDeferred() {
        publishAndProcess(publishedEvents);
        publishedEvents.clear();
    }

    void assertIsChanged(bool isChanged) {
        process();
        std::lock_guard lock{mutex};

        REQUIRE((isChanged ? listenerNotifications > 0 : listenerNotifications == 0));
        listenerNotifications = 0;
    }

    void assertIsSnapshot(bool isSnapshot) {
        std::lock_guard lock{mutex};

        REQUIRE((isSnapshot ? snapshotNotifications > 0 : snapshotNotifications == 0));
        snapshotNotifications = 0;
    }

    void assertSnapshotNotification(int count) {
        std::lock_guard lock{mutex};

        REQUIRE(snapshotNotifications == count);
        snapshotNotifications = 0;
    }

    void assertReceivedEventCount(std::size_t count) {
        std::lock_guard lock{mutex};

        REQUIRE(receivedEvents.size() == count);
    }

    void assertEvent(std::int64_t index, double size, std::int32_t eventFlags) {
        std::lock_guard lock{mutex};

        REQUIRE_FALSE(receivedEvents.empty());

        const auto order = receivedEvents.front();

        receivedEvents.pop_front();
        INFO("order: ", order->toString());
        REQUIRE(order->getEventSymbol() == SYMBOL);
        REQUIRE(order->getIndex() == index);
        REQUIRE(order->getSize() == size);
        REQUIRE(order->getEventFlags() == eventFlags);
    }
};

// Not EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END: they are statics of the library, not constants.
constexpr std::int32_t SNAPSHOT_BEGIN_END = 0x04 | 0x08;

} // namespace

TEST_CASE_FIXTURE(IndexedTxModelFixture, "IndexedTxModel: initial state") {
    REQUIRE_THROWS(IndexedTxModel<Order>::newBuilder()->build()); // the symbol is not set

    model = IndexedTxModel<Order>::newBuilder()->withSymbol(SYMBOL)->build();

    REQUIRE(model->isBatchProcessing());
    REQUIRE_FALSE(model->isSnapshotProcessing());
}

TEST_CASE_FIXTURE(IndexedTxModelFixture, "IndexedTxModel: change sources") {
    const auto dex = createOrder(0, OrderSource::DEX, 1, SNAPSHOT_BEGIN_END);
    const auto ntvLower = createOrder(0, OrderSource::ntv, 2, SNAPSHOT_BEGIN_END);
    const auto ntv = createOrder(0, OrderSource::NTV, 3, SNAPSHOT_BEGIN_END);

    model = builder()
                ->withSources({OrderSource::AGGREGATE_ASK, OrderSource::AGGREGATE_BID}) // add two sources
                ->withSources({OrderSource::ntv, OrderSource::NTV})                     // override previous sources
                ->build();

    publishAndProcess(dex); // publish an unsubscribed source
    assertIsChanged(false);

    publishAndProcess(std::vector<std::shared_ptr<Order>>{ntvLower, ntv, dex}); // two subscribed, one unsubscribed
    assertIsChanged(true);
    assertSnapshotNotification(2);
    assertReceivedEventCount(2);

    model->setSources({OrderSource::DEX}); // change source

    publishAndProcess(dex); // publish a subscribed source
    assertIsChanged(true);
    assertSnapshotNotification(1);
    assertReceivedEventCount(3);
}

TEST_CASE_FIXTURE(IndexedTxModelFixture, "IndexedTxModel: empty sources subscribe to all sources") {
    const auto dex = createOrder(0, OrderSource::DEX, 1, SNAPSHOT_BEGIN_END);
    const auto ntvLower = createOrder(0, OrderSource::ntv, 2, SNAPSHOT_BEGIN_END);
    const auto ntv = createOrder(0, OrderSource::NTV, 3, SNAPSHOT_BEGIN_END);

    model = builder()->build();

    publishAndProcess(std::vector<std::shared_ptr<Order>>{ntvLower, ntv, dex});
    assertIsChanged(true);
    assertSnapshotNotification(3);
    assertReceivedEventCount(3);
}

// The common scenarios run for each combination of the batch and snapshot processing modes.
template <typename Scenario> void forEachProcessingMode(Scenario &&scenario) {
    for (const auto isBatchProcessing : {false, true}) {
        for (const auto isSnapshotProcessing : {false, true}) {
            CAPTURE(isBatchProcessing);
            CAPTURE(isSnapshotProcessing);

            IndexedTxModelFixture fixture;

            scenario(fixture, isBatchProcessing, isSnapshotProcessing);
        }
    }
}

TEST_CASE("IndexedTxModel: attach and detach the feed") {
    forEachProcessingMode([](IndexedTxModelFixture &f, bool isBatchProcessing, bool isSnapshotProcessing) {
        const auto expectedFlags = isSnapshotProcessing ? 0 : SNAPSHOT_BEGIN_END;

        // create a model instance without attaching it to a feed
        f.model = f.builder(false)
                      ->withBatchProcessing(isBatchProcessing)
                      ->withSnapshotProcessing(isSnapshotProcessing)
                      ->build();

        f.addToPublish(0, 1, SNAPSHOT_BEGIN_END);
        f.publishDeferred();
        f.assertIsChanged(false); // the model is not attached to the feed

        f.model->attach(f.feed);
        f.addToPublish(0, 1, SNAPSHOT_BEGIN_END);
        f.publishDeferred();
        f.assertIsChanged(true);
        f.assertReceivedEventCount(1);

        f.model->detach(f.feed);
        f.addToPublish(0, 2, SNAPSHOT_BEGIN_END);
        f.publishDeferred();
        f.assertIsChanged(false); // the model is detached

        f.model->attach(f.feed);
        f.addToPublish(0, 3, SNAPSHOT_BEGIN_END);
        f.publishDeferred();
        f.assertIsChanged(true);
        f.assertReceivedEventCount(2);
        f.assertEvent(0, 1, expectedFlags);
        f.assertEvent(0, 3, expectedFlags);
    });
}

TEST_CASE("IndexedTxModel: no notifications after close") {
    forEachProcessingMode([](IndexedTxModelFixture &f, bool isBatchProcessing, bool isSnapshotProcessing) {
        f.model = f.builder()
                      ->withBatchProcessing(isBatchProcessing)
                      ->withSnapshotProcessing(isSnapshotProcessing)
                      ->build();

        f.addToPublish(0, 12.34, SNAPSHOT_BEGIN_END);
        f.publishDeferred();
        f.assertIsChanged(true);
        f.assertIsSnapshot(true);

        f.model->close();

        f.addToPublish(2, 56.78, 0); // emulate stale events processing
        f.publishDeferred();
        f.assertIsChanged(false);
    });
}
