// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "PriceLevel.hpp"

#include <atomic>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

/**
 * Represents a price level book (market by price) that aggregates individual orders (market by order).
 * For this model, use market by order sources such as dxfcpp::OrderSource::NTV", dxfcpp::OrderSource::GLBX, etc.
 *
 * @tparam O The type of the order.
 */
template <dxfcpp::Derived<dxfcpp::OrderBase> O> struct PriceLevelBook : dxfcpp::RequireMakeShared<PriceLevelBook<O>> {
    using ListenerSignature = void(const std::vector<std::shared_ptr<PriceLevel>> & /* buy */,
                                   const std::vector<std::shared_ptr<PriceLevel>> & /* sell */);

    /// Builder class for constructing instances of PriceLevelBook.
    struct Builder : dxfcpp::RequireMakeShared<Builder> {
        friend struct PriceLevelBook;

        private:
        std::shared_ptr<typename dxfcpp::IndexedTxModel<O>::Builder> builder_{};
        dxfcpp::IndexedEventSource source_;
        std::function<ListenerSignature> listener_{};
        std::size_t depthLimit_{};
        std::int64_t aggregationPeriodMillis_{};

        public:
        explicit Builder(dxfcpp::RequireMakeShared<Builder>::LockExternalConstructionTag) {
            builder_ = dxfcpp::IndexedTxModel<O>::newBuilder();
            source_ = dxfcpp::IndexedEventSource::DEFAULT;
        }

        ~Builder() override = default;

        /**
         * Sets the DXFeed for the model being created.
         * The feed cannot be attached after the model has been built.
         *
         * @param feed The DXFeed.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFeed(std::shared_ptr<dxfcpp::DXFeed> feed) {
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
        std::shared_ptr<Builder> withSymbol(const dxfcpp::SymbolWrapper &symbol) {
            builder_ = builder_->withSymbol(symbol);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * @param listener The listener.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withListener(std::function<ListenerSignature> listener) {
            listener_ = std::move(listener);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the source from which to subscribe for indexed events.
         * The listener cannot be changed or added once the model has been built.
         *
         * @param source The specified source.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withSource(const dxfcpp::IndexedEventSource &source) {
            source_ = source;
            builder_ = builder_->withSources({source});

            return this->template sharedAs<Builder>();
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
         * Builds and returns an instance of PriceLevelBook based on the provided parameters.
         *
         * @return A new instance of PriceLevelBook.
         */
        std::shared_ptr<PriceLevelBook> build() {
            return PriceLevelBook::create(this->template sharedAs<Builder>());
        }
    };

    struct BuyLess {
        bool operator()(const std::shared_ptr<PriceLevel> &pl1, const std::shared_ptr<PriceLevel> &pl2) const {
            return pl1->getPrice() > pl2->getPrice();
        }
    };

    struct SellLess {
        bool operator()(const std::shared_ptr<PriceLevel> &pl1, const std::shared_ptr<PriceLevel> &pl2) const {
            return pl1->getPrice() < pl2->getPrice();
        }
    };

    /**
     * Represents a set of price levels, sorted by a comparator.
     *
     * @tparam Less The comparator type.
     */
    template <typename Less> struct SortedPriceLevelSet {
        private:
        mutable std::recursive_mutex mutex_{};
        std::vector<std::shared_ptr<PriceLevel>> snapshot_{};
        std::set<std::shared_ptr<PriceLevel>, Less> priceLevels_{};
        std::atomic<std::size_t> depthLimit_{};
        std::atomic<bool> isChanged_{};

        bool isDepthLimitUnbounded() const {
            auto temp = depthLimit_.load();

            return temp == 0 || temp == std::numeric_limits<decltype(temp)>::max();
        }

        bool isPriceLevelCountWithinDepthLimit() const {
            std::lock_guard lock(mutex_);

            return priceLevels_.size() <= depthLimit_;
        }

        bool isPriceLevelWithinDepthLimit(const std::shared_ptr<PriceLevel> &pl) const {
            std::lock_guard lock(mutex_);

            if (snapshot_.empty()) {
                return true;
            }

            return !Less{}(snapshot_.back(), pl);
        }

        void updateSnapshot() {
            std::lock_guard lock(mutex_);

            isChanged_ = false;
            snapshot_.clear();

            const auto limit = isDepthLimitUnbounded() ? std::numeric_limits<std::size_t>::max() : depthLimit_.load();
            std::size_t i = 0;

            for (auto it = priceLevels_.begin(); i < limit && it != priceLevels_.end(); ++it) {
                snapshot_.push_back(*it);
                ++i;
            }
        }

        void markAsChangedIfNeeded(const std::shared_ptr<PriceLevel> &pl) {
            std::lock_guard lock(mutex_);

            if (isChanged_) {
                return;
            }

            if (isDepthLimitUnbounded() || isPriceLevelCountWithinDepthLimit() || isPriceLevelWithinDepthLimit(pl)) {
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
         * Returns the depth limit.
         *
         * @return the depth limit.
         */
        std::size_t getDepthLimit() const {
            return depthLimit_;
        }

        /**
         * Inserts a new price level or increases the size of an existing price level.
         *
         * @param pl The price level.
         */
        void insertOrIncrease(const std::shared_ptr<PriceLevel> &pl) {
            std::lock_guard lock(mutex_);

            auto it = priceLevels_.find(pl);

            if (it != priceLevels_.end()) {
                auto existing = *it;

                priceLevels_.erase(it);
                existing->setSize(existing->getSize() + pl->getSize());
                priceLevels_.emplace(std::make_shared<PriceLevel>(*existing));
                markAsChangedIfNeeded(existing);
            } else {
                priceLevels_.emplace(pl);
                markAsChangedIfNeeded(pl);
            }
        }

        /**
         * Removes existing price level or decreases the size of an existing price level.
         *
         * @param pl The price level.
         */
        void eraseOrDecrease(const std::shared_ptr<PriceLevel> &pl) {
            std::lock_guard lock(mutex_);

            auto it = priceLevels_.find(pl);

            if (it != priceLevels_.end()) {
                auto existing = *it;
                auto newSize = existing->getSize() - pl->getSize();

                priceLevels_.erase(it);

                if (newSize > 0.0) {
                    existing->setSize(newSize);
                    priceLevels_.emplace(std::make_shared<PriceLevel>(*existing));
                }

                markAsChangedIfNeeded(existing);
            }
        }

        /**
         * Clears price levels from the set by source.
         *
         * @param source The source to clear price levels by.
         */
        void clearBySource(const dxfcpp::IndexedEventSource &source) {
            std::lock_guard lock(mutex_);

            std::size_t size = priceLevels_.size();

            for (auto it = priceLevels_.begin(); it != priceLevels_.end(); ++it) {
                if ((*it)->getEventSource() == source) {
                    it = priceLevels_.erase(it);
                }
            }

            isChanged_ = priceLevels_.size() != size;
        }

        /**
         * Converts the set to a vector.
         *
         * @return The vector of price levels.
         */
        std::vector<std::shared_ptr<PriceLevel>> toVector() {
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
    SortedPriceLevelSet<BuyLess> buyPriceLevels_{};
    SortedPriceLevelSet<SellLess> sellPriceLevels_{};
    std::shared_ptr<dxfcpp::IndexedTxModel<O>> indexedTxModel_{};
    std::function<ListenerSignature> listener_{};
    std::size_t depthLimit_{};
    std::int64_t aggregationPeriodMillis_{};
    std::atomic<bool> taskScheduled_{};
    std::shared_ptr<dxfcpp::Timer> taskTimer_{};

    static std::shared_ptr<PriceLevelBook> create(std::shared_ptr<Builder> builder) {
        auto plb = PriceLevelBook::createShared(builder);

        plb->indexedTxModel_ =
            builder->builder_
                ->withListener([m = plb->weak_from_this()](const dxfcpp::IndexedEventSource &source,
                                                           const std::vector<std::shared_ptr<O>> &events,
                                                           bool isSnapshot) {
                    if (const auto model = m.lock()) {
                        model->template sharedAs<PriceLevelBook>()->eventsReceived(source, events, isSnapshot);
                    }
                })
                ->build();

        return plb;
    }

    static bool shallAdd(const std::shared_ptr<O> &order) {
        return order->hasSize() && !order->getEventFlagsMask().contains(dxfcpp::EventFlag::REMOVE_EVENT);
    }

    void eventsReceived(const dxfcpp::IndexedEventSource &source, const std::vector<std::shared_ptr<O>> &events,
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

        listener_(buyPriceLevels_.toVector(), sellPriceLevels_.toVector());
        taskScheduled_ = false;
    }

    void scheduleTaskIfNeeded(std::chrono::milliseconds delay) {
        std::lock_guard guard(mtx_);

        if (!taskScheduled_) {
            taskScheduled_ = true;
            taskTimer_ = dxfcpp::Timer::runOnce(
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

    bool update(const dxfcpp::IndexedEventSource &source, const std::vector<std::shared_ptr<O>> &events,
                bool isSnapshot) {
        std::lock_guard guard(mtx_);

        if (isSnapshot) {
            clearBySource(source);
        }

        for (const auto &order : events) {
            if (ordersByIndex_.contains(order->getIndex())) {
                auto removed = ordersByIndex_[order->getIndex()];

                ordersByIndex_.erase(order->getIndex());

                if (removed->getOrderSide() == dxfcpp::Side::BUY) {
                    buyPriceLevels_.eraseOrDecrease(std::make_shared<PriceLevel>(removed));
                } else {
                    sellPriceLevels_.eraseOrDecrease(std::make_shared<PriceLevel>(removed));
                }
            }

            if (shallAdd(order)) {
                ordersByIndex_[order->getIndex()] = order;

                if (order->getOrderSide() == dxfcpp::Side::BUY) {
                    buyPriceLevels_.insertOrIncrease(std::make_shared<PriceLevel>(order));
                } else {
                    sellPriceLevels_.insertOrIncrease(std::make_shared<PriceLevel>(order));
                }
            }
        }

        return buyPriceLevels_.isChanged() || sellPriceLevels_.isChanged();
    }

    void clearBySource(const dxfcpp::IndexedEventSource &source) {
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

        buyPriceLevels_.clearBySource(source);
        sellPriceLevels_.clearBySource(source);
    }

    public:
    PriceLevelBook(typename dxfcpp::RequireMakeShared<PriceLevelBook>::LockExternalConstructionTag,
                   const std::shared_ptr<Builder> &builder) {
        depthLimit_ = builder->depthLimit_;
        buyPriceLevels_.setDepthLimit(depthLimit_);
        sellPriceLevels_.setDepthLimit(depthLimit_);
        listener_ = builder->listener_;
        aggregationPeriodMillis_ = builder->aggregationPeriodMillis_;
    }

    ~PriceLevelBook() override {
        close();
    }

    /**
     * Creates a new builder instance for constructing a PriceLevelBook.
     *
     * @return A new instance of the builder.
     */
    static std::shared_ptr<Builder> newBuilder() {
        return Builder::createShared();
    }

    /**
     * @return The depth limit of the book.
     */
    std::size_t getDepthLimit() const {
        std::lock_guard guard(mtx_);

        return depthLimit_;
    }

    /**
     * Sets the depth limit of the book.
     *
     * @param depthLimit The new depth limit value.
     */
    void setDepthLimit(std::size_t depthLimit) {
        std::lock_guard guard(mtx_);

        if (depthLimit == depthLimit_) {
            return;
        }

        depthLimit_ = depthLimit;
        buyPriceLevels_.setDepthLimit(depthLimit);
        sellPriceLevels_.setDepthLimit(depthLimit);
        tryCancelTask();
        notifyListeners();
    }

    /**
     * @return The aggregation period of the book.
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
     * Closes this book and makes it <i>permanently detached</i>.
     */
    void close() const {
        std::lock_guard guard(mtx_);

        indexedTxModel_->close();
    }
};
