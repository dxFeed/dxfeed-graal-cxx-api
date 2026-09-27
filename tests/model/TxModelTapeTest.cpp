// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The common scenarios of the Java API tests of the tx models, which pass the
// events to the model directly (processEvents). Here each step writes the events to a text tape and a STREAM_FEED
// endpoint reads it: the stream contract delivers the events as they are, with their event flags, like processEvents.
// Each scenario runs for IndexedTxModel<Order> in the four processing modes; the expectations are those of Java.
// TimeSeriesTxModel is not tested this way: a STREAM_FEED endpoint has no history contract, so time series
// subscriptions get no events (a FEED endpoint processes the snapshots in QD, which is not what processEvents does).
// The publisher-based scenarios of TimeSeriesTxModel are in TimeSeriesTxModelTest.cpp.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TapeSupport.hpp"
#include "../support/TestSupport.hpp"

#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

namespace {

constexpr auto SYMBOL = "TEST-SYMBOL";
constexpr auto SENTINEL_SYMBOL = "SENTINEL";

constexpr std::int32_t TX_PENDING = 0x01;
constexpr std::int32_t REMOVE_EVENT = 0x02;
constexpr std::int32_t SNAPSHOT_BEGIN = 0x04;
constexpr std::int32_t SNAPSHOT_END = 0x08;
constexpr std::int32_t SNAPSHOT_SNIP = 0x10;

struct IndexedModel {
    using Event = Order;
    using Model = IndexedTxModel<Order>;

    static std::shared_ptr<Event> createEvent(std::int64_t index, double size, std::int32_t eventFlags) {
        auto order = std::make_shared<Order>(SYMBOL);

        order->setIndex(index);
        order->setSource(OrderSource::DEFAULT);
        order->setSize(size);
        order->setEventFlags(eventFlags);
        order->setOrderSide(Side::BUY);

        return order;
    }

    template <typename Listener>
    static std::shared_ptr<Model> build(const std::shared_ptr<DXFeed> &feed, bool isBatch, bool isSnapshot,
                                        Listener &&listener) {
        return Model::newBuilder()
            ->withFeed(feed)
            ->withSymbol(SYMBOL)
            ->withBatchProcessing(isBatch)
            ->withSnapshotProcessing(isSnapshot)
            ->withListener([listener](const IndexedEventSource &, const std::vector<std::shared_ptr<Event>> &events,
                                      bool snapshot) {
                listener(events, snapshot);
            })
            ->build();
    }
};

template <typename M> struct TapeModelFixture {
    using Event = typename M::Event;

    bool isBatchProcessing;
    bool isSnapshotProcessing;
    std::shared_ptr<InPlaceExecutor> executor = InPlaceExecutor::create();
    std::shared_ptr<DXEndpoint> endpoint = createEndpoint(executor);
    std::shared_ptr<DXFeed> feed = endpoint->getFeed();

    std::mutex mutex{};
    int listenerNotifications{};
    int snapshotNotifications{};
    std::deque<std::shared_ptr<Event>> receivedEvents{};
    std::vector<std::shared_ptr<EventType>> publishedEvents{};
    std::shared_ptr<typename M::Model> model{};
    std::shared_ptr<DXFeedSubscription> sentinelSubscription{};
    std::int64_t sentinelIndex{};
    std::int64_t receivedSentinels{};

    static std::shared_ptr<DXEndpoint> createEndpoint(const std::shared_ptr<InPlaceExecutor> &executor) {
        auto e = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::STREAM_FEED)->build();

        e->executor(executor);

        return e;
    }

    TapeModelFixture(bool isBatch, bool isSnapshot) : isBatchProcessing(isBatch), isSnapshotProcessing(isSnapshot) {
        model = M::build(feed, isBatch, isSnapshot, [this](const std::vector<std::shared_ptr<Event>> &events, bool s) {
            std::lock_guard lock{mutex};

            ++listenerNotifications;

            if (s) {
                ++snapshotNotifications;
            }

            receivedEvents.insert(receivedEvents.end(), events.begin(), events.end());
        });
        sentinelSubscription = feed->createSubscription(Order::TYPE);
        sentinelSubscription->addEventListener<Order>([this](const auto &orders) {
            std::lock_guard lock{mutex};

            receivedSentinels += static_cast<std::int64_t>(orders.size());
        });
        sentinelSubscription->addSymbols(IndexedEventSubscriptionSymbol(SENTINEL_SYMBOL, OrderSource::DEFAULT));
    }

    ~TapeModelFixture() {
        sentinelSubscription->close();
        model->close();
        endpoint->close();
    }

    void addToPublish(std::int64_t index, double size, std::int32_t eventFlags = 0) {
        publishedEvents.push_back(M::createEvent(index, size, eventFlags));
    }

    // Writes the added events to a tape and delivers them to the model through the STREAM_FEED endpoint. The tape ends
    // with an order of the SENTINEL symbol: when it is received, the events before it have been delivered as well.
    void publishDeferred() {
        const auto path = test::temporaryFilePath("tx-model-step.txt");
        auto sentinel = std::make_shared<Order>(SENTINEL_SYMBOL);

        sentinel->setIndex(++sentinelIndex);
        sentinel->setOrderSide(Side::BUY);
        publishedEvents.push_back(sentinel);
        test::writeTextTape(path, publishedEvents);
        publishedEvents.clear();

        const auto expectedSentinels = sentinelIndex;

        endpoint->connect("file:" + path + "[speed=max]");
        REQUIRE(test::waitUntil([&] {
            executor->processAllPendingTasks();

            std::lock_guard lock{mutex};

            return receivedSentinels == expectedSentinels;
        }));
        REQUIRE(test::waitUntil([&] {
            executor->processAllPendingTasks();

            return endpoint->getState() == DXEndpoint::State::NOT_CONNECTED;
        }));
        executor->processAllPendingTasks();
    }

    void assertIsChanged(bool isChanged) {
        std::lock_guard lock{mutex};

        REQUIRE((isChanged ? listenerNotifications > 0 : listenerNotifications == 0));
        listenerNotifications = 0;
    }

    void assertIsSnapshot(bool isSnapshot) {
        std::lock_guard lock{mutex};

        REQUIRE((isSnapshot ? snapshotNotifications > 0 : snapshotNotifications == 0));
        snapshotNotifications = 0;
    }

    void assertListenerNotification(int count) {
        std::lock_guard lock{mutex};

        REQUIRE(listenerNotifications == count);
        listenerNotifications = 0;
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

        const auto event = receivedEvents.front();

        receivedEvents.pop_front();
        INFO("event: ", event->toString());
        REQUIRE(event->getEventSymbol() == SYMBOL);
        REQUIRE(event->getIndex() == index);

        if (std::isnan(size)) {
            REQUIRE(std::isnan(event->getSize()));
        } else {
            REQUIRE(event->getSize() == size);
        }

        REQUIRE(event->getEventFlags() == eventFlags);
    }

    std::int32_t snapshotFlags(std::int32_t flags) const {
        return isSnapshotProcessing ? 0 : flags;
    }
};

template <typename M, typename Scenario> void forEachMode(Scenario &&scenario) {
    for (const auto isBatchProcessing : {false, true}) {
        for (const auto isSnapshotProcessing : {false, true}) {
            CAPTURE(isBatchProcessing);
            CAPTURE(isSnapshotProcessing);

            TapeModelFixture<M> f{isBatchProcessing, isSnapshotProcessing};

            scenario(f);
        }
    }
}

template <typename M> void snapshotAndUpdate(TapeModelFixture<M> &f) {
    f.addToPublish(1, 1, SNAPSHOT_BEGIN);
    f.addToPublish(0, 2, SNAPSHOT_END);
    f.addToPublish(1, 3);
    f.addToPublish(3, 4);
    f.addToPublish(2, 5);
    f.publishDeferred();
    f.assertSnapshotNotification(1); // only one snapshot
    f.assertListenerNotification(f.isBatchProcessing ? 2 : 4);
    f.assertReceivedEventCount(5);
    f.assertEvent(1, 1, f.snapshotFlags(SNAPSHOT_BEGIN));
    f.assertEvent(0, 2, f.snapshotFlags(SNAPSHOT_END));
    f.assertEvent(1, 3, 0); // the event with index 1 is not merged
    f.assertEvent(3, 4, 0);
    f.assertEvent(2, 5, 0);
}

template <typename M> void emptySnapshot(TapeModelFixture<M> &f) {
    f.addToPublish(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END | REMOVE_EVENT);
    f.publishDeferred();
    f.assertSnapshotNotification(1);

    if (f.isSnapshotProcessing) {
        f.assertReceivedEventCount(0); // the removed event is dropped inside the snapshot
    } else {
        f.assertReceivedEventCount(1);
        f.assertEvent(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END | REMOVE_EVENT);
    }
}

template <typename M> void snapshotWithPending(TapeModelFixture<M> &f) {
    f.addToPublish(1, 1, SNAPSHOT_BEGIN);
    f.publishDeferred();
    f.assertIsChanged(false); // not processed yet
    f.assertReceivedEventCount(0);

    f.addToPublish(0, 2, SNAPSHOT_END | TX_PENDING);
    f.publishDeferred();
    f.assertIsChanged(false); // the pending flag is set

    f.addToPublish(1, 3); // an event without pending
    f.publishDeferred();
    f.assertListenerNotification(1); // the transaction ended here
    f.assertSnapshotNotification(1);

    if (f.isSnapshotProcessing) {
        f.assertReceivedEventCount(2); // the same indices within a snapshot are merged
        f.assertEvent(1, 3, 0);
        f.assertEvent(0, 2, 0);
    } else {
        f.assertReceivedEventCount(3);
        f.assertEvent(1, 1, SNAPSHOT_BEGIN);
        f.assertEvent(0, 2, SNAPSHOT_END | TX_PENDING);
        f.assertEvent(1, 3, 0);
    }
}

template <typename M> void multipleSnapshot(TapeModelFixture<M> &f) {
    f.addToPublish(1, 1, SNAPSHOT_BEGIN);
    f.publishDeferred();
    f.assertIsChanged(false);
    f.assertReceivedEventCount(0);

    f.addToPublish(0, 2, SNAPSHOT_END);
    f.addToPublish(2, 3, SNAPSHOT_BEGIN);
    f.publishDeferred();
    f.assertListenerNotification(1);
    f.assertSnapshotNotification(1); // the beginning of the second snapshot is in the buffer
    f.assertReceivedEventCount(2);
    f.assertEvent(1, 1, f.snapshotFlags(SNAPSHOT_BEGIN));
    f.assertEvent(0, 2, f.snapshotFlags(SNAPSHOT_END));

    f.addToPublish(0, 4, SNAPSHOT_END); // the end of the second snapshot
    f.addToPublish(3, 5);               // an update after the second snapshot
    f.publishDeferred();
    f.assertSnapshotNotification(1);
    f.assertListenerNotification(2);
    f.assertReceivedEventCount(3);
    f.assertEvent(2, 3, f.snapshotFlags(SNAPSHOT_BEGIN));
    f.assertEvent(0, 4, f.snapshotFlags(SNAPSHOT_END));
    f.assertEvent(3, 5, 0);
}

template <typename M> void multipleSnapshotInOneBatch(TapeModelFixture<M> &f) {
    f.addToPublish(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END);
    f.addToPublish(0, 2, SNAPSHOT_BEGIN | SNAPSHOT_SNIP);
    f.addToPublish(0, 3, SNAPSHOT_BEGIN | REMOVE_EVENT | SNAPSHOT_SNIP | SNAPSHOT_END);
    f.publishDeferred();
    f.assertListenerNotification(3);
    f.assertSnapshotNotification(3);

    if (f.isSnapshotProcessing) {
        f.assertReceivedEventCount(2); // no event with REMOVE_EVENT
        f.assertEvent(0, 1, 0);
        f.assertEvent(0, 2, 0);
    } else {
        f.assertReceivedEventCount(3);
        f.assertEvent(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END);
        f.assertEvent(0, 2, SNAPSHOT_BEGIN | SNAPSHOT_SNIP);
        f.assertEvent(0, 3, SNAPSHOT_BEGIN | REMOVE_EVENT | SNAPSHOT_SNIP | SNAPSHOT_END);
    }
}

template <typename M> void incompleteSnapshot(TapeModelFixture<M> &f) {
    f.addToPublish(1, 1, SNAPSHOT_BEGIN);
    f.publishDeferred();
    f.assertIsChanged(false);
    f.assertReceivedEventCount(0);

    f.addToPublish(2, 2, SNAPSHOT_BEGIN); // yet another snapshot begins
    f.addToPublish(3, 3);                 // a part of that snapshot
    f.publishDeferred();
    f.assertIsChanged(false);
    f.assertReceivedEventCount(0);

    f.addToPublish(4, 4, SNAPSHOT_BEGIN); // a new snapshot
    f.publishDeferred();
    f.assertIsChanged(false);
    f.assertReceivedEventCount(0);

    f.addToPublish(0, 5, SNAPSHOT_END); // the full snapshot
    f.addToPublish(5, 6);               // an update after the end of the snapshot in the same batch
    f.addToPublish(6, 7);               // yet another update in the same batch
    f.publishDeferred();
    f.assertListenerNotification(f.isBatchProcessing ? 2 : 3);
    f.assertSnapshotNotification(1);
    f.assertReceivedEventCount(4); // the parts of the previous snapshots are dropped
    f.assertEvent(4, 4, f.snapshotFlags(SNAPSHOT_BEGIN));
    f.assertEvent(0, 5, f.snapshotFlags(SNAPSHOT_END));
    f.assertEvent(5, 6, 0);
    f.assertEvent(6, 7, 0);

    f.addToPublish(7, 4, SNAPSHOT_BEGIN); // the snapshot has not ended yet
    f.publishDeferred();
    f.assertIsChanged(false);
}

template <typename M> void pending(TapeModelFixture<M> &f) {
    f.addToPublish(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END);
    f.publishDeferred();
    f.assertIsChanged(true);
    f.assertIsSnapshot(true);
    f.assertReceivedEventCount(1);

    f.addToPublish(1, 2, TX_PENDING);
    f.addToPublish(2, 3, TX_PENDING);
    f.publishDeferred();
    f.assertIsChanged(false); // not processed yet

    f.addToPublish(3, 4, 0);
    f.addToPublish(4, 5, 0);
    f.publishDeferred();
    f.assertListenerNotification(f.isBatchProcessing ? 1 : 2);
    f.assertIsSnapshot(false);
    f.assertReceivedEventCount(5); // all the events, without merging
    f.assertEvent(0, 1, f.snapshotFlags(SNAPSHOT_BEGIN | SNAPSHOT_END));
    f.assertEvent(1, 2, TX_PENDING);
    f.assertEvent(2, 3, TX_PENDING);
    f.assertEvent(3, 4, 0);
    f.assertEvent(4, 5, 0);
}

template <typename M> void eventsWithoutSnapshot(TapeModelFixture<M> &f) {
    f.addToPublish(2, 1);
    f.addToPublish(3, 2);
    f.addToPublish(1, 3);
    f.addToPublish(1, 4); // the same index as the previous one
    f.addToPublish(0, 5);
    f.publishDeferred();
    f.assertListenerNotification(f.isBatchProcessing ? 1 : 5);
    f.assertReceivedEventCount(5); // the events before a snapshot are passed as they are
    f.assertEvent(2, 1, 0);
    f.assertEvent(3, 2, 0);
    f.assertEvent(1, 3, 0);
    f.assertEvent(1, 4, 0);
    f.assertEvent(0, 5, 0);

    f.addToPublish(0, 1, SNAPSHOT_BEGIN | SNAPSHOT_END);
    f.addToPublish(1, 2);
    f.publishDeferred();
    f.assertIsChanged(true);
    f.assertReceivedEventCount(2);
    f.assertEvent(0, 1, f.snapshotFlags(SNAPSHOT_BEGIN | SNAPSHOT_END));
    f.assertEvent(1, 2, 0);
}

template <typename M> void snapshotWithRemoveAndPending(TapeModelFixture<M> &f) {
    f.addToPublish(7, 1, SNAPSHOT_BEGIN);
    f.addToPublish(6, 2);
    f.addToPublish(5, 3, REMOVE_EVENT);
    f.addToPublish(4, 4);
    f.addToPublish(3, 5);
    f.addToPublish(2, 6, TX_PENDING);
    f.addToPublish(2, 7);
    f.addToPublish(1, 8);
    f.addToPublish(0, std::nan(""), SNAPSHOT_END | TX_PENDING | REMOVE_EVENT);
    f.addToPublish(1, 9);
    f.publishDeferred();

    if (f.isSnapshotProcessing) {
        f.assertReceivedEventCount(6);
        f.assertEvent(7, 1, 0);
        f.assertEvent(6, 2, 0);
        f.assertEvent(4, 4, 0);
        f.assertEvent(3, 5, 0);
        f.assertEvent(2, 7, 0);
        f.assertEvent(1, 9, 0);
    } else {
        f.assertReceivedEventCount(10);
        f.assertEvent(7, 1, SNAPSHOT_BEGIN);
        f.assertEvent(6, 2, 0);
        f.assertEvent(5, 3, REMOVE_EVENT);
        f.assertEvent(4, 4, 0);
        f.assertEvent(3, 5, 0);
        f.assertEvent(2, 6, TX_PENDING);
        f.assertEvent(2, 7, 0);
        f.assertEvent(1, 8, 0);
        f.assertEvent(0, std::nan(""), SNAPSHOT_END | TX_PENDING | REMOVE_EVENT);
        f.assertEvent(1, 9, 0);
    }
}

} // namespace

TEST_CASE_TEMPLATE("Tx model from tapes: snapshot and update", M, IndexedModel) {
    forEachMode<M>(snapshotAndUpdate<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: empty snapshot", M, IndexedModel) {
    forEachMode<M>(emptySnapshot<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: snapshot with pending", M, IndexedModel) {
    forEachMode<M>(snapshotWithPending<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: multiple snapshots", M, IndexedModel) {
    forEachMode<M>(multipleSnapshot<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: multiple snapshots in one batch", M, IndexedModel) {
    forEachMode<M>(multipleSnapshotInOneBatch<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: incomplete snapshot", M, IndexedModel) {
    forEachMode<M>(incompleteSnapshot<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: pending", M, IndexedModel) {
    forEachMode<M>(pending<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: events without a snapshot", M, IndexedModel) {
    forEachMode<M>(eventsWithoutSnapshot<M>);
}

TEST_CASE_TEMPLATE("Tx model from tapes: snapshot with remove and pending", M, IndexedModel) {
    forEachMode<M>(snapshotWithRemoveAndPending<M>);
}
