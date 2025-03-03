// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/Handler.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

/**
 * Invoked when the order book is changed.
 *
 * The MarketDepthModelListener is used to handle notifications of changes to the market depth, including updates to the
 * buy and sell orders.
 * Use the MarketDepthModelListener::create() method, passing it a lambda, function, or function object to create an
 * instance of the listener to process or react to changes in the market order book.
 *
 * @tparam O The type of order derived from OrderBase.
 */
template <Derived<OrderBase> O>
struct DXFCPP_EXPORT MarketDepthModelListener final : RequireMakeShared<MarketDepthModelListener<O>> {
    /**
     * The listener's callback (and handler) signature.
     */
    using Signature = void(const std::vector<std::shared_ptr<O>> & /* buy */,
                           const std::vector<std::shared_ptr<O>> & /* sell */);

    protected:
    SimpleHandler<Signature> onEventsReceived_{};
    mutable std::recursive_mutex mutex_{};

    public:
    /**
     * @return A handler of the listener.
     */
    SimpleHandler<Signature> &getHandler() {
        std::lock_guard guard{mutex_};

        return onEventsReceived_;
    }

    explicit MarketDepthModelListener(RequireMakeShared<MarketDepthModelListener<O>>::LockExternalConstructionTag) {};

    ~MarketDepthModelListener() override = default;

    /**
     * Constructs the new listener from the callback.
     *
     * @param onEventsReceived The callback (a lambda, a function or a functional object) with signature
     * `void(const std::vector<std::shared_ptr<O>> buyOrders, const std::vector<std::shared_ptr<O>> buyOrders)`
     * @return the new listener.
     */
    static std::shared_ptr<MarketDepthModelListener> create(std::function<Signature> onEventsReceived) {
        auto listener = RequireMakeShared<MarketDepthModelListener<O>>::createShared();

        listener->onEventsReceived_ += std::move(onEventsReceived);

        return listener;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()