// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;

namespace {

struct Closable {
    int closes{};

    void close() {
        ++closes;
    }
};

struct ThrowingClosable {
    void close() {
        throw std::runtime_error("close failed");
    }
};

} // namespace

TEST_CASE("AutoClose closes the entity once") {
    SUBCASE("at the end of the scope") {
        const auto entity = std::make_shared<Closable>();

        {
            const AutoClose autoClose{entity};

            CHECK(autoClose);
            CHECK(autoClose.get() == entity);
            CHECK(autoClose->closes == 0);
        }

        CHECK(entity->closes == 1);
    }

    SUBCASE("after a move") {
        const auto entity = std::make_shared<Closable>();

        {
            AutoClose first{entity};
            const AutoClose second{std::move(first)};

            CHECK_FALSE(first); // NOLINT(bugprone-use-after-move): a moved AutoClose is empty
            CHECK(second.get() == entity);
        }

        CHECK(entity->closes == 1);
    }

    SUBCASE("on another thread after a move into it") {
        const auto entity = std::make_shared<Closable>();

        bool moved{};

        std::thread worker{[&moved, autoClose = AutoClose{entity}] {
            moved = static_cast<bool>(autoClose);
        }};

        worker.join();

        CHECK(moved);
        CHECK(entity->closes == 1);
    }

    SUBCASE("a move assignment closes the replaced entity") {
        const auto replaced = std::make_shared<Closable>();
        const auto entity = std::make_shared<Closable>();

        {
            AutoClose autoClose{replaced};

            autoClose = AutoClose{entity};

            CHECK(replaced->closes == 1);
            CHECK(entity->closes == 0);
        }

        CHECK(replaced->closes == 1);
        CHECK(entity->closes == 1);
    }

    SUBCASE("reset() closes at once") {
        const auto entity = std::make_shared<Closable>();
        AutoClose autoClose{entity};

        autoClose.reset();
        autoClose.reset();

        CHECK_FALSE(autoClose);
        CHECK(entity->closes == 1);
    }

    SUBCASE("release() does not close") {
        const auto entity = std::make_shared<Closable>();

        {
            AutoClose autoClose{entity};

            CHECK(autoClose.release() == entity);
            CHECK_FALSE(autoClose);
        }

        CHECK(entity->closes == 0);
    }

    SUBCASE("an empty one does nothing") {
        const AutoClose<Closable> autoClose{};

        CHECK_FALSE(autoClose);
    }
}

TEST_CASE("An exception of close() in AutoClose is reported to ErrorManager") {
    const test::ErrorManagerGuard guard{};

    { const AutoClose autoClose{std::make_shared<ThrowingClosable>()}; }

    // Only the errors of AutoClose: a callback or a release of an earlier test can report its own.
    std::vector<ErrorInfo> errors{};

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.component == "AutoClose") {
            errors.push_back(error);
        }
    }

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message == "close failed");
}

TEST_CASE("AutoClose closes a subscription and an endpoint at the end of the scope") {
    std::weak_ptr<DXEndpoint> weakEndpoint{};
    std::weak_ptr<DXFeedSubscription> subscriptionOfEndpoint{};

    {
        const AutoClose endpoint{DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::LOCAL_HUB)->build()};
        std::shared_ptr<DXFeedSubscription> subscription{};

        {
            const AutoClose autoClose{endpoint->getFeed()->createSubscription(Quote::TYPE)};

            subscription = autoClose.get();
            CHECK_FALSE(subscription->isClosed());
        }

        CHECK(subscription->isClosed());
        CHECK_FALSE(endpoint->isClosed());

        // Owned by the feed: released and closed when the endpoint is closed.
        subscriptionOfEndpoint = endpoint->getFeed()->createSubscription(Quote::TYPE);
        weakEndpoint = endpoint.get();
    }

    CHECK(test::waitUntil([&] {
        return subscriptionOfEndpoint.expired();
    }));
    CHECK(test::waitUntil([&] {
        return weakEndpoint.expired();
    }));
}
