// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <memory>
#include <set>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;

namespace {

auto subscriptionManager() {
    return ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>();
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
