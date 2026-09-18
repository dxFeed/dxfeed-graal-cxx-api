// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp>

#include <atomic>
#include <future>
#include <memory>
#include <thread>

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
