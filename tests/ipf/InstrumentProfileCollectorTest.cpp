// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;
using namespace std::literals;

namespace {

std::shared_ptr<InstrumentProfile> stock(const std::string &symbol) {
    auto profile = InstrumentProfile::create();

    profile->setType("STOCK");
    profile->setSymbol(symbol);

    return profile;
}

std::vector<std::string> viewSymbols(const InstrumentProfileCollector &collector) {
    std::vector<std::string> result{};
    const auto view = collector.view();

    while (view->hasNext()) {
        result.push_back(view->next()->getSymbol());
    }

    std::sort(result.begin(), result.end());

    return result;
}

/// Collects the type and symbol of every profile that a collector passes to its update listener.
struct UpdateLog {
    std::mutex mutex{};
    std::vector<std::string> entries{};

    void add(const std::vector<std::shared_ptr<InstrumentProfile>> &profiles) {
        std::lock_guard lock{mutex};

        for (const auto &profile : profiles) {
            entries.push_back(profile->getType() + " " + profile->getSymbol());
        }
    }

    bool contains(const std::string &entry) {
        std::lock_guard lock{mutex};

        return std::find(entries.begin(), entries.end(), entry) != entries.end();
    }
};

} // namespace

TEST_CASE("InstrumentProfileCollector::view() returns the updated profiles") {
    const auto collector = InstrumentProfileCollector::create();

    CHECK(viewSymbols(*collector).empty());

    collector->updateInstrumentProfile(stock("AAPL"));
    collector->updateInstrumentProfile(stock("MSFT"));

    CHECK(viewSymbols(*collector) == std::vector<std::string>{"AAPL", "MSFT"});
}

TEST_CASE("InstrumentProfileCollector::getLastUpdateTime() grows after an update") {
    const auto collector = InstrumentProfileCollector::create();
    const auto before = collector->getLastUpdateTime();

    std::this_thread::sleep_for(50ms);
    collector->updateInstrumentProfile(stock("AAPL"));

    CHECK(collector->getLastUpdateTime() > before);
}

TEST_CASE("InstrumentProfileCollector passes added and removed profiles to a listener until it is removed") {
    const auto collector = InstrumentProfileCollector::create();
    UpdateLog removedLog{};
    UpdateLog keptLog{};

    const auto removed = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &ips) {
        removedLog.add(ips);
    });
    const auto kept = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &ips) {
        keptLog.add(ips);
    });

    collector->updateInstrumentProfile(stock("AAPL"));

    CHECK(test::waitUntil([&] {
        return removedLog.contains("STOCK AAPL") && keptLog.contains("STOCK AAPL");
    }));

    const auto removal = InstrumentProfile::create();

    removal->setType("REMOVED");
    removal->setSymbol("AAPL");
    collector->updateInstrumentProfile(removal);

    CHECK(test::waitUntil([&] {
        return removedLog.contains("REMOVED AAPL") && keptLog.contains("REMOVED AAPL");
    }));

    collector->removeUpdateListener(removed);
    collector->updateInstrumentProfile(stock("MSFT"));

    // The kept listener is called for the update; the removed one is not called for it.
    CHECK(test::waitUntil([&] {
        return keptLog.contains("STOCK MSFT");
    }));
    CHECK_FALSE(removedLog.contains("STOCK MSFT"));

    collector->removeUpdateListener(kept);
}

// MDAPI-429: the collector gives the ids of its update listeners itself; each listener has a handler of its own, and
// the ids of the handlers are unique only within a handler.
TEST_CASE("InstrumentProfileCollector gives different ids to the update listeners and calls each of them") {
    const auto collector = InstrumentProfileCollector::create();
    std::atomic<int> firstCalls{};
    std::atomic<int> secondCalls{};

    const auto first = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        ++firstCalls;
    });
    const auto second = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        ++secondCalls;
    });

    CHECK(first != second);

    const auto profile = InstrumentProfile::create();

    profile->setType("STOCK");
    profile->setSymbol("AAPL");
    collector->updateInstrumentProfile(profile);

    CHECK(test::waitUntil([&] {
        return firstCalls > 0 && secondCalls > 0;
    }));

    collector->removeUpdateListener(first);
    collector->removeUpdateListener(second);
}

// MDAPI-441: the collector calls its listeners without holding its lock, so a listener can remove itself.
TEST_CASE("A listener of InstrumentProfileCollector can remove itself") {
    const auto collector = InstrumentProfileCollector::create();
    std::atomic<std::size_t> listenerId{};
    std::atomic<bool> removed{false};

    listenerId = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        if (!removed.exchange(true)) {
            collector->removeUpdateListener(listenerId.load());
        }
    });

    const auto profile = InstrumentProfile::create();

    profile->setType("STOCK");
    profile->setSymbol("AAPL");
    collector->updateInstrumentProfile(profile);

    CHECK(test::waitUntil([&] {
        return removed.load();
    }));

    // The collector still works: a new listener gets the profiles.
    std::atomic<int> otherCalls{};

    const auto other = collector->addUpdateListener([&](const std::vector<std::shared_ptr<InstrumentProfile>> &) {
        ++otherCalls;
    });

    CHECK(test::waitUntil([&] {
        return otherCalls > 0;
    }));

    collector->removeUpdateListener(other);
}
