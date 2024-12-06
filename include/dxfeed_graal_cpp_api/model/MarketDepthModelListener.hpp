// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Id.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT MarketDepthModelListenerCommon : virtual SharedEntity {
    using Signature = void(const std::vector<std::shared_ptr<OrderBase>> & /* buy */,
                           const std::vector<std::shared_ptr<OrderBase>> & /* sell */);

  protected:
    SimpleHandler<Signature> onEventsReceived_{};
    mutable std::recursive_mutex mutex_{};

  public:
    SimpleHandler<Signature> &getHandler() {
        std::lock_guard guard{mutex_};

        return onEventsReceived_;
    }
};

template <Derived<OrderBase> E>
struct DXFCPP_EXPORT MarketDepthModelListener final : MarketDepthModelListenerCommon,
                                                      RequireMakeShared<MarketDepthModelListener<E>> {
    explicit MarketDepthModelListener(RequireMakeShared<MarketDepthModelListener<E>>::LockExternalConstructionTag) {};

    ~MarketDepthModelListener() override = default;

    static std::shared_ptr<MarketDepthModelListener<E>>
    create(std::function<void(const std::vector<std::shared_ptr<E>> & /* buy */,
                              const std::vector<std::shared_ptr<E>> & /* sell */)>
               onEventsReceived) {
        auto listener = RequireMakeShared<MarketDepthModelListener<E>>::createShared();

        listener->onEventsReceived_ += [l = onEventsReceived](const std::vector<std::shared_ptr<OrderBase>> &buy,
                                                              const std::vector<std::shared_ptr<OrderBase>> &sell) {
            std::vector<std::shared_ptr<E>> buyOrders{};
            std::vector<std::shared_ptr<E>> sellOrders{};

            buyOrders.reserve(buy.size());
            sellOrders.reserve(sell.size());

            for (const auto &buyOrder : buy) {
                buyOrders.emplace_back(buyOrder->template sharedAs<E>());
            }

            for (const auto &sellOrder : sell) {
                sellOrders.emplace_back(sellOrder->template sharedAs<E>());
            }

            l(buyOrders, sellOrders);
        };

        return listener;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()