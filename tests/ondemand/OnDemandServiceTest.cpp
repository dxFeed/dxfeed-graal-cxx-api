// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <memory>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"

using namespace dxfcpp;

namespace {

std::shared_ptr<DXEndpoint> onDemandFeed() {
    return DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::ON_DEMAND_FEED)->build();
}

} // namespace

TEST_CASE("The on-demand service of an endpoint is one instance") {
    const auto endpoint = onDemandFeed();
    const auto service = OnDemandService::getInstance(endpoint);

    CHECK(OnDemandService::getInstance(endpoint) == service);
    CHECK(service->getEndpoint() == endpoint);

    endpoint->close();
}

TEST_CASE("Closed endpoints are released with their on-demand services") {
    std::vector<std::weak_ptr<DXEndpoint>> endpoints{};
    std::vector<std::weak_ptr<OnDemandService>> services{};

    // A Java endpoint takes milliseconds to create: 500 are enough to see a leak.
    for (int i = 0; i < 500; ++i) {
        const auto endpoint = onDemandFeed();
        const auto service = OnDemandService::getInstance(endpoint);

        endpoint->close();

        if (i % 50 == 0) {
            endpoints.push_back(endpoint);
            services.push_back(service);
        }
    }

    // The CLOSED notification may come after close() returns and keep the endpoint for a while.
    CHECK(test::waitUntil([&] {
        return std::ranges::all_of(endpoints, [](const auto &endpoint) {
            return endpoint.expired();
        });
    }));
    CHECK(std::ranges::all_of(services, [](const auto &service) {
        return service.expired();
    }));
}

TEST_CASE("The on-demand service of a released endpoint returns no endpoint") {
    auto endpoint = onDemandFeed();
    const std::weak_ptr<DXEndpoint> weak = endpoint;
    const auto service = OnDemandService::getInstance(endpoint);

    endpoint->close();
    endpoint.reset();

    REQUIRE(test::waitUntil([&] {
        return weak.expired();
    }));
    CHECK(service->getEndpoint() == nullptr);
}
