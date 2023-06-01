// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <memory>

#include "../managers/DXEndpointManager.hpp"
#include "../managers/DXFeedSubscriptionManager.hpp"

namespace dxfcpp {

class DXFCPP_EXPORT ApiContext {
    mutable std::shared_ptr<DXEndpointManager> dxEndpointManager_;
    mutable std::shared_ptr<DXFeedSubscriptionManager> dxFeedSubscriptionManager_;
    std::atomic<bool> initialized{false};

    ApiContext() noexcept
        : dxEndpointManager_{std::make_shared<DXEndpointManager>()},
          dxFeedSubscriptionManager_{std::make_shared<DXFeedSubscriptionManager>()} {}

  public:
    static std::shared_ptr<ApiContext> getInstance() noexcept {
        static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new (std::nothrow) ApiContext{});

        return instance;
    }

    std::shared_ptr<DXEndpointManager> getDxEndpointManager() const noexcept { return dxEndpointManager_; }

    std::shared_ptr<DXFeedSubscriptionManager> getDxFeedSubscriptionManager() const noexcept {
        return dxFeedSubscriptionManager_;
    }
};

} // namespace dxfcpp