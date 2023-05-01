// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <memory>

#pragma once

namespace dxfcpp {

class DXEndpointManager;

class ApiContext {
    std::shared_ptr<DXEndpointManager> dxEndpointManager_;
    std::atomic<bool> initialized{false};

    ApiContext() {
        dxEndpointManager_ = {};
    }

  public:


    static std::shared_ptr<ApiContext> init() {
        return {};
    }

    std::shared_ptr<DXEndpointManager> getDxEndpointManager() {
        if (initialized) {
            return dxEndpointManager_;
        }

        init();


    }
};

}