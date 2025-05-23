// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <set>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/market/Order.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../internal/Timer.hpp"
#include "IndexedTxModel.hpp"
#include "MarketDepthModelListener.hpp"

#include <memory>
#include <unordered_set>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;
struct SymbolWrapper;

/**
 * Represents a model for market depth, tracking buy and sell orders and notifies listener of changes in the order book.
 *
 * This model can set depth limit and aggregation period. This model notifies the user of received transactions through
 * an installed MarketDepthModelListener.
 *
 * The depth limit specifies the maximum number of buy or sell orders to maintain in the order book.
 * For example, if the depth limit is set to 10, the model will only keep track of the top 10 buy orders and the top 10
 * sell orders. This helps in managing the size of the order book.
 *
 * The aggregation period, specified in milliseconds, determines the frequency at which the model aggregates and
 * notifies changes in the order book to the listeners. For instance, if the aggregation period is set to 1000
 * milliseconds the model will aggregate changes and notify listeners every second. A value of 0 means that changes are
 * notified immediately.
 *
 * <h3>Configuration</h3>
 *
 * This model must be configured using the @ref MarketDepthModel::Builder "Builder" class, as most configuration
 * settings cannot be changed once the model is built. This model requires configuration
 * MarketDepthModel::Builder::withSymbol() and it must be MarketDepthModel::Builder::withFeed() attached to a DXFeed
 * instance to begin operation.
 *
 * This model only supports single symbol subscriptions; multiple symbols cannot be configured.
 *
 * The convenient way to detach model from the feed is to call its MarketDepthModel::close() method. Closed model
 * becomes permanently detached from all feeds, removes all its listeners.
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 *
 * Sample:
 *
 * ```cpp
 * using namespace std::literals;
 *
 * auto ep = DXEndpoint::getInstance();
 * auto feed = ep->getFeed();
 *
 * auto model =
 *     MarketDepthModel<Order>::newBuilder()
 *         ->withFeed(feed)
 *         ->withSources({OrderSource::NTV})
 *         ->withSymbol("AAPL")
 *         ->withDepthLimit(10)
 *         ->withAggregationPeriod(5s)
 *         ->withListener([](const std::vector<std::shared_ptr<Order>> &buy,
 *                           const std::vector<std::shared_ptr<Order>> &sell) {
 *             if (buy.empty() && sell.empty()) {
 *                 return;
 *             }
 *
 *             std::cout << std::format("{:=^66}\n", "");
 *             std::cout << std::format("{:^31} || {:^31}\n", "ASK", "BID");
 *             std::cout << std::format("{0:^15}|{1:^15} || {0:^15}|{1:^15}\n", "Price", "Size");
 *             std::cout << std::format("{:-^66}\n", "");
 *
 *             for (auto buyIt = buy.begin(), sellIt = sell.begin(); buyIt != buy.end() || sellIt != sell.end();) {
 *                 std::string row{};
 *                 if (buyIt != buy.end()) {
 *                     row += std::format("{:>14.4f} | {:<14.2f}", (*buyIt)->getPrice(), (*buyIt)->getSize());
 *
 *                     ++buyIt;
 *                 } else {
 *                     row += std::format("{:>14} | {:<14}", "", "");
 *                 }
 *
 *                 row += " || ";
 *
 *                 if (sellIt != sell.end()) {
 *                     row += std::format("{:>14.4f} | {:<14.2f}", (*sellIt)->getPrice(), (*sellIt)->getSize());
 *
 *                     ++sellIt;
 *                 } else {
 *                     row += std::format("{:>14} | {:<14}", "", "");
 *                 }
 *
 *                 std::cout << row << std::endl;
 *             }
 *
 *             std::cout << std::format("{:=^66}\n", "");
 *         })
 *         ->build();
 *
 * ep->connect("demo.dxfeed.com:7300");
 * ```
 *
 * @tparam O The type of order derived from OrderBase.
 */
template <Derived<OrderBase> O> struct /* DXFCPP_EXPORT */ MarketDepthModel final : RequireMakeShared<MarketDepthModel<O>> {

    struct /* DXFCPP_EXPORT */ Builder final : RequireMakeShared<Builder> {
        friend struct MarketDepthModel;

        private:
        std::shared_ptr<typename IndexedTxModel<O>::Builder> builder_{};
        std::shared_ptr<MarketDepthModelListener<O>> listener_{};
        std::size_t depthLimit_{};
        std::int64_t aggregationPeriodMillis_{};

        public:
        explicit Builder(RequireMakeShared<Builder>::LockExternalConstructionTag) {
            builder_ = IndexedTxModel<O>::newBuilder();
        }

        ~Builder() override = default;

        /**
         * Sets the DXFeed for the model being created.
         * The feed cannot be attached after the model has been built.
         *
         * @param feed The DXFeed.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFeed(std::shared_ptr<DXFeed> feed) {
            builder_ = builder_->withFeed(std::move(feed));

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the subscription symbol for the model being created.
         * The symbol cannot be added or changed after the model has been built.
         *
         * @param symbol The subscription symbol.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol) {
            builder_ = builder_->withSymbol(symbol);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * @param listener The transaction listener.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withListener(std::shared_ptr<MarketDepthModelListener<O>> listener) {
            this->listener_ = listener;

            return this->template sharedAs<Builder>();
        }

        /**
         * Creates and sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * @param onEventsReceived The callback.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withListener(std::function<void(const std::vector<std::shared_ptr<O>> & /* buy */,
                                                                 const std::vector<std::shared_ptr<O>> & /* sell */)>
                                                  onEventsReceived) {
            this->listener_ = MarketDepthModelListener<O>::create(onEventsReceived);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * @remark The default value for this source is an empty set, which means that this model subscribes to all
         * available sources.
         * @tparam EventSourceIt The source collection iterator type.
         * @param begin The beginning of the collection of sources.
         * @param end The end of the collection of sources.
         * @return The builder instance.
         */
        template <typename EventSourceIt> std::shared_ptr<Builder> withSources(EventSourceIt begin, EventSourceIt end) {
            builder_ = builder_->withSources(begin, end);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * @remark The default value for this source is an empty set, which means that this model subscribes to all
         * available sources.
         *
         * @tparam EventSourceCollection A type of the collection of sources (std::vector<EventSourceWrapper>,
         * std::set<OrderSource>, etc.)
         * @param sources The specified sources.
         * @return The builder instance.
         */
        template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
        std::shared_ptr<Builder> withSources(EventSourceCollection &&sources) {
            return withSources(std::begin(sources), std::end(sources));
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * @remark The default value for this source is an empty set, which means that this model subscribes to all
         * available sources.
         *
         * @param sources The specified sources.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withSources(std::initializer_list<EventSourceWrapper> sources) {
            return withSources(sources.begin(), sources.end());
        }

        /**
         * Sets the depth limit.
         *
         * @param depthLimit The depth limit.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withDepthLimit(std::size_t depthLimit) {
            depthLimit_ = depthLimit;

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the aggregation period.
         *
         * @param aggregationPeriodMillis The aggregation period in milliseconds.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::int64_t aggregationPeriodMillis) {
            aggregationPeriodMillis_ = aggregationPeriodMillis;

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the aggregation period.
         *
         * @param aggregationPeriod The aggregation period.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
            return withAggregationPeriod(aggregationPeriod.count());
        }

        /**
         * Builds an instance of MarketDepthModel based on the provided parameters.
         *
         * @return The created MarketDepthModel.
         */
        std::shared_ptr<MarketDepthModel> build() {
            return MarketDepthModel::create(this->template sharedAs<Builder>());
        }
    };

    struct OrderComparator {
        int operator()(const std::shared_ptr<O> &o1, const std::shared_ptr<O> &o2) const {
            auto ind1 = o1->getScope() == Scope::ORDER;
            auto ind2 = o2->getScope() == Scope::ORDER;

            if (ind1 && ind2) {
                // Both orders are individual orders.
                auto c = math::compare(o1->getTimeSequence(), o2->getTimeSequence()); // asc

                if (c != 0) {
                    return c;
                }

                c = math::compare(o1->getIndex(), o2->getIndex()); // asc

                return c;
            }

            if (ind1) {
                // First order is individual, second is not.
                return 1;
            }

            if (ind2) {
                // Second order is individual, first is not.
                return -1;
            }

            // Both orders are non-individual orders.
            auto c = math::compare(o2->getSize(), o1->getSize()); // desc

            if (c != 0) {
                return c;
            }

            c = math::compare(o1->getTimeSequence(), o2->getTimeSequence()); // asc

            if (c != 0) {
                return c;
            }

            c = math::compare(o1->getScope().getCode(), o2->getScope().getCode()); // asc

            if (c != 0) {
                return c;
            }

            c = math::compare(o1->getExchangeCode(), o2->getExchangeCode()); // asc

            if (c != 0) {
                return c;
            }

            if (o1->template is<Order>() && o2->template is<Order>()) {
                auto order1 = o1->template sharedAs<Order>();
                auto order2 = o2->template sharedAs<Order>();

                if (order1->getMarketMaker() < order2->getMarketMaker()) {
                    return -1;
                }

                if (order1->getMarketMaker() > order2->getMarketMaker()) {
                    return 1;
                }
            }

            c = math::compare(o1->getIndex(), o2->getIndex()); // asc

            return c;
        }
    };

    struct BuyComparator {
        int operator()(const std::shared_ptr<O> &o1, const std::shared_ptr<O> &o2) const {
            if (o1->getPrice() < o2->getPrice()) {
                return 1; // desc
            }

            if (o1->getPrice() > o2->getPrice()) {
                return -1;
            }

            return OrderComparator{}(o1, o2);
        }
    };

    struct SellComparator {
        int operator()(const std::shared_ptr<O> &o1, const std::shared_ptr<O> &o2) const {
            if (o1->getPrice() < o2->getPrice()) {
                return -1; // asc
            }

            if (o1->getPrice() > o2->getPrice()) {
                return 1;
            }

            return OrderComparator{}(o1, o2);
        }
    };

    struct BuyLess {
        bool operator()(const std::shared_ptr<O> &o1, const std::shared_ptr<O> &o2) const {
            return BuyComparator{}(o1, o2) < 0;
        }
    };

    struct SellLess {
        bool operator()(const std::shared_ptr<O> &o1, const std::shared_ptr<O> &o2) const {
            return SellComparator{}(o1, o2) < 0;
        }
    };

    /**
     * Represents a set of orders, sorted by a comparator.
     * @tparam Less The comparator type.
     */
    template <typename Less> struct SortedOrderSet {
        private:
        mutable std::recursive_mutex mutex_{};
        std::vector<std::shared_ptr<O>> snapshot_{};
        std::set<std::shared_ptr<O>, Less> orders_{};
        std::atomic<std::size_t> depthLimit_{};
        std::atomic<bool> isChanged_{};

        bool isDepthLimitUnbounded() const {
            auto temp = depthLimit_.load();

            return temp == 0 || temp == std::numeric_limits<decltype(temp)>::max();
        }

        bool isOrderCountWithinDepthLimit() const {
            std::lock_guard lock(mutex_);

            return orders_.size() <= depthLimit_;
        }

        bool isOrderWithinDepthLimit(const std::shared_ptr<O> &order) const {
            std::lock_guard lock(mutex_);

            if (snapshot_.empty()) {
                return true;
            }

            return !Less{}(snapshot_.back(), order);
        }

        void updateSnapshot() {
            std::lock_guard lock(mutex_);

            isChanged_ = false;
            snapshot_.clear();

            const auto limit = isDepthLimitUnbounded() ? std::numeric_limits<std::size_t>::max() : depthLimit_.load();
            std::size_t i = 0;

            for (auto it = orders_.begin(); i < limit && it != orders_.end(); ++it) {
                snapshot_.push_back(*it);
                ++i;
            }
        }

        void markAsChangedIfNeeded(const std::shared_ptr<O> &order) {
            std::lock_guard lock(mutex_);

            if (isChanged_) {
                return;
            }

            if (isDepthLimitUnbounded() || isOrderCountWithinDepthLimit() || isOrderWithinDepthLimit(order)) {
                isChanged_ = true;
            }
        }

        public:
        /// @return A value indicating whether this set has changed.
        bool isChanged() const {
            return isChanged_;
        }

        /**
         * Sets the depth limit.
         *
         * @param depthLimit The new depth limit.
         */
        void setDepthLimit(std::size_t depthLimit) {
            std::lock_guard lock(mutex_);

            if (depthLimit_ == depthLimit) {
                return;
            }

            depthLimit_ = depthLimit;
            isChanged_ = true;
        }

        /**
         * Inserts an order to set.
         *
         * @param order The order to add.
         * @return `true` if order was added.
         */
        bool insert(const std::shared_ptr<O> &order) {
            std::lock_guard lock(mutex_);

            if (orders_.insert(order).second) {
                markAsChangedIfNeeded(order);

                return true;
            }

            return false;
        }

        /**
         * Removes an order from the set.
         *
         * @param order The order to remove.
         * @return `true` if order was removed.
         */
        bool erase(const std::shared_ptr<O> &order) {
            std::lock_guard lock(mutex_);

            if (orders_.erase(order) > 0) {
                markAsChangedIfNeeded(order);

                return true;
            }

            return false;
        }

        /**
         * Clears orders from the set by source.
         *
         * @param source The source to clear orders by.
         */
        void clearBySource(const IndexedEventSource &source) {
            std::lock_guard lock(mutex_);

            std::size_t size = orders_.size();

            for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                if ((*it)->getSource() == source) {
                    it = orders_.erase(it);
                }
            }

            isChanged_ = orders_.size() != size;
        }

        std::vector<std::shared_ptr<O>> toVector() {
            std::lock_guard lock(mutex_);

            if (isChanged_) {
                updateSnapshot();
            }

            return {snapshot_.begin(), snapshot_.end()};
        }
    };

    private:
    mutable std::recursive_mutex mtx_{};
    std::unordered_map<std::int64_t, std::shared_ptr<O>> ordersByIndex_{};
    SortedOrderSet<BuyLess> buyOrders_{};
    SortedOrderSet<SellLess> sellOrders_{};
    std::shared_ptr<IndexedTxModel<O>> indexedTxModel_{};
    std::shared_ptr<MarketDepthModelListener<O>> listener_{};
    std::size_t depthLimit_{};
    std::int64_t aggregationPeriodMillis_{};
    std::atomic<bool> taskScheduled_{};
    std::shared_ptr<Timer> taskTimer_{};

    static std::shared_ptr<MarketDepthModel> create(std::shared_ptr<Builder> builder) {
        auto marketDepthModel = MarketDepthModel::createShared(builder);

        marketDepthModel->indexedTxModel_ =
            builder->builder_
                ->withListener([m = marketDepthModel->weak_from_this()](const IndexedEventSource &source,
                                                                        const std::vector<std::shared_ptr<O>> &events,
                                                                        bool isSnapshot) {
                    if (const auto model = m.lock()) {
                        model->template sharedAs<MarketDepthModel>()->eventsReceived(source, events, isSnapshot);
                    }
                })
                ->build();

        return marketDepthModel;
    }

    static bool shallAdd(const std::shared_ptr<O> &order) {
        return order->hasSize() && !order->getEventFlagsMask().contains(EventFlag::REMOVE_EVENT);
    }

    void eventsReceived(const IndexedEventSource &source, const std::vector<std::shared_ptr<O>> &events,
                        bool isSnapshot) {
        std::lock_guard guard(mtx_);

        if (!update(source, events, isSnapshot)) {
            return;
        }

        if (isSnapshot || aggregationPeriodMillis_ == 0) {
            tryCancelTask();
            notifyListeners();
        } else {
            scheduleTaskIfNeeded(std::chrono::milliseconds(aggregationPeriodMillis_));
        }
    }

    void notifyListeners() {
        std::lock_guard guard(mtx_);

        listener_->getHandler()(getBuyOrders(), getSellOrders());
        taskScheduled_ = false;
    }

    void scheduleTaskIfNeeded(std::chrono::milliseconds delay) {
        std::lock_guard guard(mtx_);

        if (!taskScheduled_) {
            taskScheduled_ = true;
            taskTimer_ = Timer::runOnce(
                [this] {
                    notifyListeners();
                },
                delay);
        }
    }

    void rescheduleTaskIfNeeded(std::chrono::milliseconds delay) {
        std::lock_guard guard(mtx_);

        if (tryCancelTask() && delay.count() != 0) {
            scheduleTaskIfNeeded(delay);
        }
    }

    bool tryCancelTask() {
        std::lock_guard guard(mtx_);

        if (taskScheduled_ && taskTimer_ && taskTimer_->isRunning()) {
            taskTimer_->stop();
            taskTimer_.reset();
            taskScheduled_ = false;

            return true;
        }

        return false;
    }

    bool update(const IndexedEventSource &source, const std::vector<std::shared_ptr<O>> &events, bool isSnapshot) {
        std::lock_guard guard(mtx_);

        if (isSnapshot) {
            clearBySource(source);
        }

        for (const auto &order : events) {
            if (ordersByIndex_.contains(order->getIndex())) {
                auto removed = ordersByIndex_[order->getIndex()];

                ordersByIndex_.erase(order->getIndex());

                if (removed->getOrderSide() == Side::BUY) {
                    buyOrders_.erase(removed);
                } else {
                    sellOrders_.erase(removed);
                }
            }

            if (shallAdd(order)) {
                ordersByIndex_[order->getIndex()] = order;

                if (order->getOrderSide() == Side::BUY) {
                    buyOrders_.insert(order);
                } else {
                    sellOrders_.insert(order);
                }
            }
        }

        return buyOrders_.isChanged() || sellOrders_.isChanged();
    }

    std::vector<std::shared_ptr<O>> getBuyOrders() {
        std::lock_guard guard(mtx_);

        return buyOrders_.toVector();
    }

    std::vector<std::shared_ptr<O>> getSellOrders() {
        std::lock_guard guard(mtx_);

        return sellOrders_.toVector();
    }

    void clearBySource(const IndexedEventSource &source) {
        std::lock_guard guard(mtx_);

        std::vector<std::int64_t> indices{};
        indices.reserve(ordersByIndex_.size());

        for (const auto &[index, order] : ordersByIndex_) {
            if (order->getSource() == source) {
                indices.push_back(index);
            }
        }

        for (auto index : indices) {
            ordersByIndex_.erase(index);
        }

        buyOrders_.clearBySource(source);
        sellOrders_.clearBySource(source);
    }

    public:
    MarketDepthModel(typename RequireMakeShared<MarketDepthModel<O>>::LockExternalConstructionTag,
                     const std::shared_ptr<Builder> &builder) {
        depthLimit_ = builder->depthLimit_;
        buyOrders_.setDepthLimit(depthLimit_);
        sellOrders_.setDepthLimit(depthLimit_);
        aggregationPeriodMillis_ = builder->aggregationPeriodMillis_;
        listener_ = builder->listener_;
    }

    ~MarketDepthModel() override {
        close();
    }

    /**
     * Creates a new builder instance for constructing a MarketDepthModel.
     *
     * @return A new instance of the builder.
     */
    static std::shared_ptr<Builder> newBuilder() {
        return Builder::createShared();
    }

    /**
     * @return The depth limit of the model.
     */
    std::size_t getDepthLimit() const {
        std::lock_guard guard(mtx_);

        return depthLimit_;
    }

    /**
     * Sets the depth limit of the model.
     *
     * @param depthLimit The new depth limit value.
     */
    void setDepthLimit(std::size_t depthLimit) {
        std::lock_guard guard(mtx_);

        if (depthLimit == depthLimit_) {
            return;
        }

        depthLimit_ = depthLimit;
        buyOrders_.setDepthLimit(depthLimit);
        sellOrders_.setDepthLimit(depthLimit);
        tryCancelTask();
        notifyListeners();
    }

    /**
     * @return The aggregation period of the model.
     */
    std::int64_t getAggregationPeriod() const {
        std::lock_guard guard(mtx_);

        return aggregationPeriodMillis_;
    }

    /**
     * Sets the aggregation period in milliseconds.
     *
     * @param aggregationPeriodMillis The new aggregation period value.
     */
    void setAggregationPeriod(std::int64_t aggregationPeriodMillis) {
        std::lock_guard guard(mtx_);

        if (aggregationPeriodMillis == aggregationPeriodMillis_) {
            return;
        }

        aggregationPeriodMillis_ = aggregationPeriodMillis;
        rescheduleTaskIfNeeded(std::chrono::milliseconds(aggregationPeriodMillis_));
    }

    /**
     * Sets the aggregation period.
     *
     * @param aggregationPeriod The new aggregation period value.
     */
    void setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
        setAggregationPeriod(aggregationPeriod.count());
    }

    /**
     * Closes this model and makes it <i>permanently detached</i>.
     */
    void close() const {
        std::lock_guard guard(mtx_);

        indexedTxModel_->close();
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()