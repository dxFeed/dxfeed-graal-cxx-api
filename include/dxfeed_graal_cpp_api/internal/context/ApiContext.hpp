// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

namespace dxfcpp {

class DXEndpointManager;
class DXFeedSubscriptionManager;

class ApiContext {
    std::shared_ptr<DXEndpointManager> dxEndpointManager_;
    std::shared_ptr<DXFeedSubscriptionManager> dxFeedSubscriptionManager_;
    std::atomic<bool> initialized{false};

    ApiContext() : dxEndpointManager_{}, dxFeedSubscriptionManager_{} {}

  public:
    static std::shared_ptr<ApiContext> getInstance() {
        static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new ApiContext{});

        return instance;
    }

    std::shared_ptr<DXEndpointManager> getDxEndpointManager() { return dxEndpointManager_; }

    std::shared_ptr<DXFeedSubscriptionManager> getDxFeedSubscriptionManager() { return dxFeedSubscriptionManager_; }
};

} // namespace dxfcpp