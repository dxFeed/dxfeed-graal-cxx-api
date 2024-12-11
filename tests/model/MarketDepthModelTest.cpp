// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dxfeed_graal_cpp_api/model/MarketDepthModel.hpp"

#include <random>
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
    std::shared_ptr<MarketDepthModel<Order>> model_{};

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

    void checkChanged(bool expected) {
        REQUIRE(expected ? listenerCalls_ > 0 : listenerCalls_ == 0);

        listenerCalls_ = 0;
    }

    void checkBookSize(int size) {
        REQUIRE_EQ(buyOrders_.size() + sellOrders_.size(), size);
    }

    void checkBuySize(int size) {
        REQUIRE_EQ(buyOrders_.size(), size);
    }

    void checkSellSize(int size) {
        REQUIRE_EQ(sellOrders_.size(), size);
    }

    void checkChangesBuy(int n) {
        REQUIRE_EQ(changesBuy_, n);

        changesBuy_ = 0;
    }

    void checkChangesSell(int n) {
        REQUIRE_EQ(changesSell_, n);

        changesSell_ = 0;
    }

    void checkOrder(const Side &side, const std::shared_ptr<Order> &order, int pos) {
        const auto &orders = side == Side::BUY ? buyOrders_ : sellOrders_;

        REQUIRE_GT(orders.size(), pos);
        REQUIRE(same(order, orders[pos]));
    }

    void publish(const std::shared_ptr<Order> &order) {
        publishedEvents_.push_back(order);
    }

    void publish(const std::vector<std::shared_ptr<Order>> &orders) {
        publishedEvents_.insert(publishedEvents_.end(), std::begin(orders), std::end(orders));
    }

    void process() {
        publisher_->publishEvents(publishedEvents_);
        executor_->processAllPendingTasks();
        publishedEvents_.clear();
    }

    void publishAndProcess(const std::shared_ptr<Order> &order) {
        publish(order);
        process();
    }

    void publishAndProcess(bool expected, const std::shared_ptr<Order> &order) {
        publish(order);
        process();
        checkChanged(expected);
    }

    void publishAndProcess(const std::vector<std::shared_ptr<Order>> &orders) {
        publish(orders);
        process();
    }

    void publishAndProcess(bool expected, const std::vector<std::shared_ptr<Order>> &orders) {
        publish(orders);
        process();
        checkChanged(expected);
    }

    static int oneIfBuy(const std::shared_ptr<Order> &order) {
        return order && order->getOrderSide() == Side::BUY && !math::equals(order->getSize(), 0) ? 1 : 0;
    }

    static int oneIfSell(const std::shared_ptr<Order> &order) {
        return order && order->getOrderSide() == Side::SELL && !math::equals(order->getSize(), 0) ? 1 : 0;
    }

    static bool same(const std::shared_ptr<Order> &order, const std::shared_ptr<Order> &old) {
        if (order && order->getSize() == 0) {
            return true; // order with zero size is the same as null (missing)
        }

        if (!order && !old) {
            return true;
        }

        if (!order || !old) {
            return false;
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

    std::shared_ptr<MarketDepthModel<Order>::Builder> createBuilder() {
        return MarketDepthModel<Order>::newBuilder()
            ->withFeed(feed_)
            ->withSymbol(symbol_)
            ->withSources({OrderSource::DEFAULT})
            ->withListener([&](const std::vector<std::shared_ptr<Order>> &buyOrders,
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

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestRemoveNonExistent") {
    publishAndProcess(false, createOrder(0, Side::BUY, 1, 1, EventFlag::REMOVE_EVENT.getFlag()));
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestRemoveBySizeAndByFlags") {
    auto o1 = createOrder(2, Side::BUY, 3, 1, 0);
    auto o2 = createOrder(1, Side::BUY, 2, 1, 0);
    auto o3 = createOrder(0, Side::BUY, 1, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask());

    publishAndProcess(true, {o3, o2, o1});
    checkBuySize(3);
    checkSellSize(0);
    checkOrder(Side::BUY, o1, 0);
    checkOrder(Side::BUY, o2, 1);
    checkOrder(Side::BUY, o3, 2);

    publishAndProcess(true, createOrder(2, Side::BUY, 1, math::NaN, 0));
    checkBuySize(2);
    checkSellSize(0);
    checkOrder(Side::BUY, o2, 0);

    publishAndProcess(true, createOrder(1, Side::BUY, 1, 1, EventFlag::REMOVE_EVENT.getFlag()));
    checkBuySize(1);
    checkSellSize(0);
    checkOrder(Side::BUY, o3, 0);

    publishAndProcess(true,
                      createOrder(0, Side::BUY, 1, 1, (EventFlag::SNAPSHOT_END | EventFlag::REMOVE_EVENT).getMask()));
    checkBookSize(0);
    checkBuySize(0);
    checkSellSize(0);
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestOrderChangeSide") {
    auto buy = createOrder(0, Side::BUY, 1, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask());

    publishAndProcess(true, buy);
    checkBookSize(1);
    checkBuySize(1);
    checkOrder(Side::BUY, buy, 0);

    auto sell = createOrder(0, Side::SELL, 1, 1, 0);

    publishAndProcess(true, sell);
    checkBookSize(1);
    checkSellSize(1);
    checkOrder(Side::SELL, sell, 0);
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestOrderPriorityAfterUpdate") {
    auto b1 = createOrder(0, Side::BUY, 100, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask());
    auto b2 = createOrder(1, Side::BUY, 150, 1, 0);
    auto s1 = createOrder(3, Side::SELL, 150, 1, 0);
    auto s2 = createOrder(2, Side::SELL, 100, 1, 0);

    publishAndProcess(b1);
    checkOrder(Side::BUY, b1, 0);
    publishAndProcess(b2);
    checkOrder(Side::BUY, b2, 0);
    checkOrder(Side::BUY, b1, 1);

    publishAndProcess(s1);
    checkOrder(Side::SELL, s1, 0);
    publishAndProcess(s2);
    checkOrder(Side::SELL, s2, 0);
    checkOrder(Side::SELL, s1, 1);
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestMultipleUpdatesWithMixedSides") {
    auto buyLowPrice =
        createOrder(0, Side::BUY, 100, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask());
    auto buyHighPrice = createOrder(1, Side::BUY, 200, 1, 0);
    auto sellLowPrice = createOrder(2, Side::SELL, 150, 1, 0);
    auto sellHighPrice = createOrder(3, Side::SELL, 250, 1, 0);

    publishAndProcess({buyLowPrice, sellHighPrice, buyHighPrice, sellLowPrice});
    checkBookSize(4);

    checkOrder(Side::BUY, buyHighPrice, 0);
    checkOrder(Side::SELL, sellLowPrice, 0);
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestDuplicateOrderIndexUpdatesExistingOrder") {
    auto originalIndexOrder =
        createOrder(0, Side::BUY, 100, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask());
    auto duplicateIndexOrder = createOrder(0, Side::BUY, 150, 1, 0);

    publishAndProcess({originalIndexOrder, duplicateIndexOrder});
    checkBookSize(1);
    checkOrder(Side::BUY, duplicateIndexOrder, 0);
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestEnforceEntryLimit") {
    model_.reset();
    model_ = createBuilder()->withDepthLimit(3)->build();

    publishAndProcess(true,
                      createOrder(0, Side::BUY, 5, 1, (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END).getMask()));
    publishAndProcess(true, createOrder(1, Side::BUY, 4, 1, 0));
    publishAndProcess(true, createOrder(2, Side::BUY, 3, 1, 0));

    publishAndProcess(false, createOrder(3, Side::BUY, 2, 1, 0));         // outside limit
    publishAndProcess(false, createOrder(4, Side::BUY, 1, 1, 0));         // outside limit
    publishAndProcess(false, createOrder(4, Side::BUY, 1, 2, 0));         // modify outside limit
    publishAndProcess(false, createOrder(3, Side::BUY, 1, math::NaN, 0)); // remove outside limit
    publishAndProcess(true, createOrder(2, Side::BUY, 3, 2, 0));          // update in limit
    publishAndProcess(true, createOrder(1, Side::BUY, 3, math::NaN, 0));  // remove in limit

    publishAndProcess(true, createOrder(4, Side::SELL, 1, 1, 0));
    publishAndProcess(true, createOrder(5, Side::SELL, 2, 1, 0));
    publishAndProcess(true, createOrder(6, Side::SELL, 3, 1, 0));

    publishAndProcess(false, createOrder(7, Side::SELL, 4, 1, 0)); // outside limit
    checkChanged(false);
    publishAndProcess(false, createOrder(8, Side::SELL, 5, 1, 0)); // outside limit
    checkChanged(false);
    publishAndProcess(false, createOrder(8, Side::SELL, 5, 2, 0)); // modify outside limit
    checkChanged(false);
    publishAndProcess(false, createOrder(8, Side::SELL, 5, math::NaN, 0)); // remove outside limit
    checkChanged(false);
    publishAndProcess(true, createOrder(6, Side::SELL, 4, 2, 0));         // update in limit
    publishAndProcess(true, createOrder(5, Side::SELL, 2, math::NaN, 0)); // remove in limit

    model_->setDepthLimit(0); // disable limit
    publishAndProcess(true, createOrder(4, Side::BUY, 1, 3, 0));
    publishAndProcess(true, createOrder(8, Side::SELL, 1, 3, 0));

    model_->setDepthLimit(1);
    publishAndProcess(true, createOrder(0, Side::BUY, 2, 1, 0));
    publishAndProcess(false, createOrder(1, Side::BUY, 2, 1, 0));
}

TEST_CASE_FIXTURE(MarketDepthModelTestFixture, "TestStressBuySellOrders") {
    publishAndProcess(
        false, createOrder(0, Side::BUY, math::NaN, math::NaN,
                           (EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END | EventFlag::REMOVE_EVENT).getMask()));
    listenerCalls_ = 0;
    buyOrders_.clear();
    publishedEvents_.clear();
    sellOrders_.clear();
    listenerCalls_ = 0;
    changesBuy_ = 0;
    changesSell_ = 0;

    const std::size_t bookSize = 100;
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> bookDistrib(0, bookSize - 1);
    std::uniform_int_distribution<> sideDistrib(0, 1);
    std::uniform_int_distribution<> valueDistrib(0, 9);

    std::vector<std::shared_ptr<Order>> book{bookSize};

    auto expectedBuy = 0;
    auto expectedSell = 0;

    for (auto i = 0; i < 10000; i++) {
        auto index = bookDistrib(gen);
        auto order = createOrder(Scope::ORDER, sideDistrib(gen) != 0 ? Side::BUY : Side::SELL, index, valueDistrib(gen),
                                 '\0', std::nullopt);
        auto old = book[index];

        book[index] = order;

        auto deltaBuy = oneIfBuy(order) - oneIfBuy(old);
        auto deltaSell = oneIfSell(order) - oneIfSell(old);

        expectedBuy += deltaBuy;
        expectedSell += deltaSell;
        publishAndProcess(order);

        if (order->getOrderSide() == Side::BUY) {
            checkChangesBuy(deltaBuy != 0 || (!same(order, old) && old->getOrderSide() == Side::BUY) ? 1 : 0);
            checkChangesSell(oneIfSell(old));
        } else if (order->getOrderSide() == Side::SELL) {
            checkChangesSell(deltaSell != 0 || (!same(order, old) && old->getOrderSide() == Side::SELL) ? 1 : 0);
            checkChangesBuy(oneIfBuy(old));
        } else {
            FAIL("Unexpected order side");
        }

        REQUIRE_EQ(buyOrders_.size(), expectedBuy);
        REQUIRE_EQ(sellOrders_.size(), expectedSell);
    }

    auto snapshotOrder = createOrder(Scope::ORDER, Side::UNDEFINED, 0, 0, '\0', std::nullopt);

    snapshotOrder->setEventFlags(EventFlag::SNAPSHOT_BEGIN | EventFlag::SNAPSHOT_END | EventFlag::REMOVE_EVENT);

    publisher_->publishEvents(snapshotOrder);
    executor_->processAllPendingTasks();

    checkChangesBuy(expectedBuy > 0 ? 1 : 0);
    checkChangesSell(expectedSell > 0 ? 1 : 0);

    REQUIRE(buyOrders_.empty());
    REQUIRE(sellOrders_.empty());
}