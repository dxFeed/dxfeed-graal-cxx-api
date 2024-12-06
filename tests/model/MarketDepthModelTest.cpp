// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dxfeed_graal_cpp_api/model/MarketDepthModel.hpp"

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

class MarketDepthModelTestFixture {
  protected:
    const char *symbol_ = "INDEX-TEST";

    std::shared_ptr<InPlaceExecutor> executor_;
    std::vector<std::shared_ptr<Order>> publishedEvents_{};
    std::vector<std::shared_ptr<Order>> buyOrders_{};
    std::vector<std::shared_ptr<Order>> sellOrders_{};
    std::shared_ptr<DXEndpoint> endpoint_{};
    std::shared_ptr<DXFeed> feed_{};
    std::shared_ptr<DXPublisher> publisher_{};
    std::shared_ptr<MarketDepthModel> model_{};

    std::size_t listenerCalls_{};
    std::size_t changesBuy_{};
    std::size_t changesSell_{};

    static bool equals(const std::vector<std::shared_ptr<Order>> &v1, const std::vector<std::shared_ptr<Order>> &v2) {
        if (v1.size() != v2.size()) {
            return false;
        }

        for (std::size_t i = 0; i < v1.size(); i++) {
            if (v1[i]->toString() != v2[i]->toString()) {
                return false;
            }
        }

        return true;
    }

    static int oneIfBuy(const std::shared_ptr<Order> & order) {
        return order->getOrderSide() == Side::BUY && ! !math::equals(order->getSize(), 0) ? 1 : 0;
    }

    static int oneIfSell(const std::shared_ptr<Order> & order) {
        return order->getOrderSide() == Side::SELL && ! !math::equals(order->getSize(), 0) ? 1 : 0;
    }

    static bool same(const std::shared_ptr<Order> &order, const std::shared_ptr<Order> &old) {
        if (order->getSize() == 0) {
            return true; // order with zero size is the same as null (missing)
        }

        // Check just relevant attributes
        return order->getScope() == old->getScope() && order->getOrderSide() == old->getOrderSide() &&
               order->getIndex() == old->getIndex() && math::equals(order->getSize(), old->getSize()) &&
               order->getSource() == old->getSource();
    }

    std::shared_ptr<Order> createOrder(std::int64_t index, const Side &side, double price, double size,
                                       std::int32_t eventFlags) {
        return std::make_shared<Order>(symbol_)
            ->withIndex(index)
            .withOrderSide(side)
            .withPrice(price)
            .withSize(size)
            .withEventFlags(eventFlags)
            .sharedAs<Order>();
    }

    std::shared_ptr<Order> createOrder(const Scope &scope, const Side &side, std::int64_t index, int value,
                                       char exchange, const std::optional<std::string> &mmid) {
        auto order = std::make_shared<Order>(symbol_)
                         ->withScope(scope)
                         .withOrderSide(side)
                         .withIndex(index)
                         .withPrice(value * 10)
                         .withSize(value)
                         .withExchangeCode(exchange)
                         .sharedAs<Order>();

        if (!mmid.has_value()) {
            return order;
        }

        return order->withMarketMaker(mmid.value()).sharedAs<Order>();
    }

    std::shared_ptr<MarketDepthModel::Builder> createBuilder() {
        return MarketDepthModel::newBuilder()
            ->withFeed(feed_)
            ->withSymbol(symbol_)
            ->withSources({OrderSource::DEFAULT})
            ->withListener<Order>([&](const std::vector<std::shared_ptr<Order>> &buyOrders,
                                      const std::vector<std::shared_ptr<Order>> &sellOrders) {
                listenerCalls_++;

                if (!equals(buyOrders_, buyOrders)) {
                    changesBuy_++;
                }

                buyOrders_ = buyOrders;

                if (!equals(sellOrders_, sellOrders)) {
                    changesSell_++;
                }

                sellOrders_ = sellOrders;
            });
    }

  public:
    MarketDepthModelTestFixture() {
        executor_ = InPlaceExecutor::create();
        endpoint_ = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
        endpoint_->executor(executor_);
        feed_ = endpoint_->getFeed();
        publisher_ = endpoint_->getPublisher();
        model_ = createBuilder()->build();
    }

    ~MarketDepthModelTestFixture() {
        listenerCalls_ = 0;
        publishedEvents_.clear();
        buyOrders_.clear();
        sellOrders_.clear();
    }
};
