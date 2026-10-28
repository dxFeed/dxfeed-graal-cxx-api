// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp>

#include <atomic>
#include <future>
#include <memory>
#include <set>
#include <thread>
#include <vector>

using namespace dxfcpp;

namespace {

struct WeakManagerTestEntity {
    explicit WeakManagerTestEntity(std::atomic_bool &destroyed) : destroyed_{destroyed} {
    }

    ~WeakManagerTestEntity() {
        destroyed_ = true;
    }

    std::atomic_bool &destroyed_;
};

} // namespace

TEST_CASE("WeakEntityManager does not own registered entities") {
    WeakEntityManager<WeakManagerTestEntity> manager;
    std::atomic_bool destroyed{false};
    auto entity = std::make_shared<WeakManagerTestEntity>(destroyed);
    const auto id = manager.registerEntity(entity);

    entity.reset();

    CHECK(destroyed.load());
    CHECK_FALSE(manager.getEntity(id));
    CHECK_FALSE(manager.contains(id));
}

TEST_CASE("WeakEntityManager keeps an entity alive for the duration of an acquired callback reference") {
    WeakEntityManager<WeakManagerTestEntity> manager;
    std::atomic_bool destroyed{false};
    auto entity = std::make_shared<WeakManagerTestEntity>(destroyed);
    const auto id = manager.registerEntity(entity);
    std::promise<void> callbackAcquiredPromise;
    auto callbackAcquired = callbackAcquiredPromise.get_future();
    std::promise<void> releaseCallbackPromise;
    auto releaseCallback = releaseCallbackPromise.get_future();
    std::atomic_bool acquired{false};

    std::thread callbackThread{[&] {
        auto callbackReference = manager.getEntity(id);
        acquired = static_cast<bool>(callbackReference);
        callbackAcquiredPromise.set_value();
        releaseCallback.wait();
    }};

    callbackAcquired.wait();
    CHECK(acquired.load());
    entity.reset();
    CHECK_FALSE(destroyed.load());

    releaseCallbackPromise.set_value();
    callbackThread.join();

    CHECK(destroyed.load());
    CHECK_FALSE(manager.getEntity(id));
}

TEST_CASE("EntityManager::size() counts registered entities") {
    EntityManager<WeakManagerTestEntity> manager;
    std::atomic_bool destroyed{false};
    auto entity = std::make_shared<WeakManagerTestEntity>(destroyed);

    CHECK(manager.size() == 0);

    const auto id = manager.registerEntity(entity);

    CHECK(manager.size() == 1);
    CHECK(manager.unregisterEntity(id));
    CHECK(manager.size() == 0);
}

TEST_CASE("WeakEntityManager::size() counts only live entities") {
    WeakEntityManager<WeakManagerTestEntity> manager;
    std::atomic_bool firstDestroyed{false};
    std::atomic_bool secondDestroyed{false};
    auto first = std::make_shared<WeakManagerTestEntity>(firstDestroyed);
    auto second = std::make_shared<WeakManagerTestEntity>(secondDestroyed);

    CHECK(manager.size() == 0);

    ignoreUnused(manager.registerEntity(first));
    const auto secondId = manager.registerEntity(second);

    CHECK(manager.size() == 2);

    first.reset();

    CHECK(manager.size() == 1);
    CHECK(manager.getEntity(secondId) == second);
}

TEST_CASE("WeakEntityManager gives unique ids to entities registered concurrently") {
    constexpr std::size_t threadCount = 8;
    constexpr std::size_t entitiesPerThread = 1000;
    WeakEntityManager<WeakManagerTestEntity> manager;
    std::atomic_bool destroyed{false};
    std::vector<std::vector<std::shared_ptr<WeakManagerTestEntity>>> entities(threadCount);
    std::vector<std::vector<std::size_t>> ids(threadCount);
    std::vector<std::thread> threads{};

    for (std::size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t] {
            for (std::size_t i = 0; i < entitiesPerThread; ++i) {
                entities[t].push_back(std::make_shared<WeakManagerTestEntity>(destroyed));
                ids[t].push_back(manager.registerEntity(entities[t].back()).getValue());
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    std::set<std::size_t> uniqueIds{};

    for (const auto &threadIds : ids) {
        uniqueIds.insert(threadIds.begin(), threadIds.end());
    }

    CHECK(uniqueIds.size() == threadCount * entitiesPerThread);
    CHECK(manager.size() == threadCount * entitiesPerThread);
}
