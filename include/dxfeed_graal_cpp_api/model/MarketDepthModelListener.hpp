// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT MarketDepthModelListenerCommon : virtual SharedEntity {

};

template <Derived<OrderBase> E>
struct DXFCPP_EXPORT MarketDepthModelListener : MarketDepthModelListenerCommon, RequireMakeShared<MarketDepthModelListener<E>> {
    static std::shared_ptr<MarketDepthModelListener<E>>
    create(std::function<void(const std::vector<std::shared_ptr<E>> & /* buy */,
                              const std::vector<std::shared_ptr<E>> & /* sell */)>
               onEventsReceived) {
        //TODO: implement
        return {};
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()