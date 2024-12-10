// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/Handler.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

template <Derived<OrderBase> O>
struct DXFCPP_EXPORT MarketDepthModelListener final : RequireMakeShared<MarketDepthModelListener<O>> {
    using Signature = void(const std::vector<std::shared_ptr<O>> & /* buy */,
                           const std::vector<std::shared_ptr<O>> & /* sell */);

  protected:
    SimpleHandler<Signature> onEventsReceived_{};
    mutable std::recursive_mutex mutex_{};

  public:
    SimpleHandler<Signature> &getHandler() {
        std::lock_guard guard{mutex_};

        return onEventsReceived_;
    }

    explicit MarketDepthModelListener(RequireMakeShared<MarketDepthModelListener<O>>::LockExternalConstructionTag) {};

    ~MarketDepthModelListener() override = default;

    static std::shared_ptr<MarketDepthModelListener<O>> create(std::function<Signature> onEventsReceived) {
        auto listener = RequireMakeShared<MarketDepthModelListener<O>>::createShared();

        listener->onEventsReceived_ += std::move(onEventsReceived);

        return listener;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()