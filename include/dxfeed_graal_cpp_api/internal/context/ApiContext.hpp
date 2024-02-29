// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>

#include "../managers/DXEndpointManager.hpp"
#include "../managers/DXFeedManager.hpp"
#include "../managers/DXFeedSubscriptionManager.hpp"
#include "../managers/DXPublisherManager.hpp"
#include "../managers/InstrumentProfileCollectorManager.hpp"
#include "../managers/InstrumentProfileConnectionManager.hpp"
#include "../managers/InstrumentProfileReaderManager.hpp"
#include "../managers/OnDemandServiceManager.hpp"

DXFCPP_BEGIN_NAMESPACE

template <typename Manager> struct AddManagerMixin {
    mutable std::shared_ptr<Manager> manager_;

    AddManagerMixin() noexcept : manager_{std::make_shared<Manager>()} {
    }

    std::shared_ptr<Manager> getManager() const noexcept {
        return manager_;
    }
};

class DXFCPP_EXPORT ApiContext : AddManagerMixin<DXEndpointManager>,
                                 AddManagerMixin<DXFeedSubscriptionManager>,
                                 AddManagerMixin<DXFeedManager>,
                                 AddManagerMixin<DXPublisherManager>,
                                 AddManagerMixin<InstrumentProfileReaderManager>,
                                 AddManagerMixin<InstrumentProfileCollectorManager>,
                                 AddManagerMixin<InstrumentProfileConnectionManager>,
                                 AddManagerMixin<OnDemandServiceManager> {
    ApiContext() noexcept = default;

  public:
    static std::shared_ptr<ApiContext> getInstance() noexcept {
        static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new (std::nothrow) ApiContext{});

        return instance;
    }

    template <typename Manager> std::shared_ptr<Manager> getManager() const noexcept {
        return AddManagerMixin<Manager>::getManager();
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()