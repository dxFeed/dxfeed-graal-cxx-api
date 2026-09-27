// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

// The scenarios follow the Java API tests of TimeSeriesTxModel, the ones that publish events through
// a LOCAL_HUB publisher. The model notifies its listener through the endpoint executor, see process().
namespace {

constexpr auto SYMBOL = "TEST-SYMBOL";

// Not EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END: they are statics of the library, not constants.
constexpr std::int32_t SNAPSHOT_BEGIN_END = 0x04 | 0x08;

struct TimeSeriesTxModelFixture : test::LocalHubFixture {
    std::mutex mutex{};
    int listenerNotifications{};
    int snapshotNotifications{};
    std::deque<std::shared_ptr<TimeAndSale>> receivedEvents{};
    std::vector<std::shared_ptr<TimeAndSale>> publishedEvents{};
    std::shared_ptr<TimeSeriesTxModel<TimeAndSale>> model{};

    ~TimeSeriesTxModelFixture() {
        if (model) {
            model->close();
        }
    }

    std::shared_ptr<TimeSeriesTxModel<TimeAndSale>::Builder> builder(bool withFeed = true) {
        auto b = TimeSeriesTxModel<TimeAndSale>::newBuilder();

        if (withFeed) {
            b = b->withFeed(feed);
        }

        return b->withSymbol(SYMBOL)->withFromTime(0)->withListener(TimeSeriesTxModelListener<TimeAndSale>::create(
            [this](const std::vector<std::shared_ptr<TimeAndSale>> &events, bool isSnapshot) {
                std::lock_guard lock{mutex};

                ++listenerNotifications;

                if (isSnapshot) {
                    ++snapshotNotifications;
                }

                receivedEvents.insert(receivedEvents.end(), events.begin(), events.end());
            }));
    }

    // Common scenarios: the index of an event is its position in the time series.
    static std::shared_ptr<TimeAndSale> createEventWithIndex(std::int64_t index, double size, std::int32_t eventFlags) {
        auto tns = std::make_shared<TimeAndSale>(SYMBOL);

        tns->setIndex(index);
        tns->setSize(size);
        tns->setEventFlags(eventFlags);

        return tns;
    }

    static std::shared_ptr<TimeAndSale> createEventWithTime(std::int64_t time, double size, std::int32_t eventFlags) {
        auto tns = std::make_shared<TimeAndSale>(SYMBOL);

        tns->setTime(time);
        tns->setSize(size);
        tns->setEventFlags(eventFlags);

        return tns;
    }

    void addToPublish(std::int64_t index, double size, std::int32_t eventFlags = 0) {
        publishedEvents.push_back(createEventWithIndex(index, size, eventFlags));
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

    void assertReceivedEventCount(std::size_t count) {
        std::lock_guard lock{mutex};

        REQUIRE(receivedEvents.size() == count);
    }

    void assertEvent(std::int64_t index, double size, std::int32_t eventFlags) {
        std::lock_guard lock{mutex};

        REQUIRE_FALSE(receivedEvents.empty());

        const auto tns = receivedEvents.front();

        receivedEvents.pop_front();
        INFO("event: ", tns->toString());
        REQUIRE(tns->getEventSymbol() == SYMBOL);
        REQUIRE(tns->getIndex() == index);
        REQUIRE(tns->getSize() == size);
        REQUIRE(tns->getEventFlags() == eventFlags);
    }
};

template <typename Scenario> void forEachProcessingMode(Scenario &&scenario) {
    for (const auto isBatchProcessing : {false, true}) {
        for (const auto isSnapshotProcessing : {false, true}) {
            CAPTURE(isBatchProcessing);
            CAPTURE(isSnapshotProcessing);

            TimeSeriesTxModelFixture fixture;

            scenario(fixture, isBatchProcessing, isSnapshotProcessing);
        }
    }
}

} // namespace

TEST_CASE_FIXTURE(TimeSeriesTxModelFixture, "TimeSeriesTxModel: initial state") {
    REQUIRE_THROWS(TimeSeriesTxModel<TimeAndSale>::newBuilder()->build()); // the symbol is not set

    model = TimeSeriesTxModel<TimeAndSale>::newBuilder()->withSymbol(SYMBOL)->build();

    REQUIRE(model->isBatchProcessing());
    REQUIRE_FALSE(model->isSnapshotProcessing());
    REQUIRE(model->getFromTime() == std::numeric_limits<std::int64_t>::max());
}

TEST_CASE_FIXTURE(TimeSeriesTxModelFixture, "TimeSeriesTxModel: change fromTime") {
    model = builder()->withFromTime(std::numeric_limits<std::int64_t>::max())->build(); // no subscription

    publishAndProcess(createEventWithTime(0, 1, SNAPSHOT_BEGIN_END));
    assertIsChanged(false); // the model is not subscribed

    model->setFromTime(0);
    publishAndProcess(createEventWithTime(0, 2, SNAPSHOT_BEGIN_END));
    assertIsChanged(true);

    model->setFromTime(1000);
    publishAndProcess(createEventWithTime(1000, 3, 0));
    assertIsChanged(true);

    model->setFromTime(1000);
    publishAndProcess(createEventWithTime(0, 3, 0));
    assertIsChanged(false); // an event before fromTime

    model->setFromTime(std::numeric_limits<std::int64_t>::max());
    publishAndProcess(createEventWithTime(0, 4, SNAPSHOT_BEGIN_END));
    assertIsChanged(false); // the model is not subscribed
}

TEST_CASE("TimeSeriesTxModel: attach and detach the feed") {
    forEachProcessingMode([](TimeSeriesTxModelFixture &f, bool isBatchProcessing, bool isSnapshotProcessing) {
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

TEST_CASE("TimeSeriesTxModel: no notifications after close") {
    forEachProcessingMode([](TimeSeriesTxModelFixture &f, bool isBatchProcessing, bool isSnapshotProcessing) {
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
