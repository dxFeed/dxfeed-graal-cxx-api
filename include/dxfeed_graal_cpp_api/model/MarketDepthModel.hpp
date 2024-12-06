// Copyright (c) 2024 Devexperts LLC.
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

struct DXFCPP_EXPORT MarketDepthModel final : RequireMakeShared<MarketDepthModel> {

    struct DXFCPP_EXPORT Builder final : RequireMakeShared<Builder> {
        friend struct MarketDepthModel;

      private:
        std::shared_ptr<IndexedTxModel::Builder> builder_{};
        std::shared_ptr<MarketDepthModelListenerCommon> listener_{};
        std::size_t depthLimit_{};
        std::int64_t aggregationPeriodMillis_{};

      public:
        explicit Builder(LockExternalConstructionTag);

        ~Builder() override;

        std::shared_ptr<Builder> withFeed(std::shared_ptr<DXFeed> feed);

        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol);

        template <Derived<OrderBase> E>
        std::shared_ptr<Builder> withListener(std::shared_ptr<MarketDepthModelListener<E>> listener) {
            // listener = listener->template sharedAs<MarketDepthModelListenerCommon>();
            this->listener_ = listener;

            return sharedAs<Builder>();
        }

        template <Derived<OrderBase> E>
        std::shared_ptr<Builder> withListener(std::function<void(const std::vector<std::shared_ptr<E>> & /* buy */,
                                                                 const std::vector<std::shared_ptr<E>> & /* sell */)>
                                                  onEventsReceived) {
            this->listener_ = MarketDepthModelListener<E>::create(onEventsReceived);

            return sharedAs<Builder>();
        }

        template <typename EventSourceIt> std::shared_ptr<Builder> withSources(EventSourceIt begin, EventSourceIt end) {
            builder_ = builder_->withSources(begin, end);

            return sharedAs<Builder>();
        }

        template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
        std::shared_ptr<Builder> withSources(EventSourceCollection &&sources) {
            return withSources(std::begin(sources), std::end(sources));
        }

        std::shared_ptr<Builder> withSources(std::initializer_list<EventSourceWrapper> sources);

        std::shared_ptr<Builder> withDepthLimit(std::size_t depthLimit);

        std::shared_ptr<Builder> withAggregationPeriod(std::int64_t aggregationPeriodMillis);

        std::shared_ptr<Builder> withAggregationPeriod(std::chrono::milliseconds aggregationPeriod);

        std::shared_ptr<MarketDepthModel> build();
    };

    struct OrderComparator {
        int operator()(const std::shared_ptr<OrderBase> &o1, const std::shared_ptr<OrderBase> &o2) const;
    };

    struct BuyComparator {
        int operator()(const std::shared_ptr<OrderBase> &o1, const std::shared_ptr<OrderBase> &o2) const;
    };

    struct SellComparator {
        int operator()(const std::shared_ptr<OrderBase> &o1, const std::shared_ptr<OrderBase> &o2) const;
    };

    struct BuyLess {
        bool operator()(const std::shared_ptr<OrderBase> &o1, const std::shared_ptr<OrderBase> &o2) const {
            return BuyComparator{}(o1, o2) < 0;
        }
    };

    struct SellLess {
        bool operator()(const std::shared_ptr<OrderBase> &o1, const std::shared_ptr<OrderBase> &o2) const {
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
        std::vector<std::shared_ptr<OrderBase>> snapshot_{};
        std::set<std::shared_ptr<OrderBase>, Less> orders_{};
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

        bool isOrderWithinDepthLimit(const std::shared_ptr<OrderBase> &order) const {
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

            auto limit = isDepthLimitUnbounded() ? std::numeric_limits<std::size_t>::max() : depthLimit_.load();
            std::size_t i = 0;

            for (auto it = orders_.begin(); i < limit && it != orders_.end(); ++it) {
                snapshot_.push_back(*it);
                ++i;
            }
        }

        void markAsChangedIfNeeded(const std::shared_ptr<OrderBase> &order) {
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
        bool insert(const std::shared_ptr<OrderBase> &order) {
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
        bool erase(const std::shared_ptr<OrderBase> &order) {
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
         * @return `true` if any order was removed.
         */
        bool clearBySource(const IndexedEventSource &source) {
            std::lock_guard lock(mutex_);

            std::size_t size = orders_.size();

            for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                if ((*it)->getSource() == source) {
                    it = orders_.erase(it);
                }
            }

            return orders_.size() != size;
        }

        std::vector<std::shared_ptr<OrderBase>> toVector() {
            std::lock_guard lock(mutex_);

            if (isChanged_) {
                updateSnapshot();
            }

            return {snapshot_.begin(), snapshot_.end()};
        }
    };

  private:
    mutable std::recursive_mutex mtx_{};
    std::unordered_map<std::int64_t, std::shared_ptr<OrderBase>> ordersByIndex_{};
    SortedOrderSet<BuyLess> buyOrders_{};
    SortedOrderSet<SellLess> sellOrders_{};
    std::shared_ptr<IndexedTxModel> indexedTxModel_{};
    std::shared_ptr<MarketDepthModelListenerCommon> listener_{};
    std::size_t depthLimit_{};
    std::int64_t aggregationPeriodMillis_{};
    std::atomic<bool> taskScheduled_{};
    std::shared_ptr<Timer> taskTimer_{};

    static std::shared_ptr<MarketDepthModel> create(std::shared_ptr<Builder> builder);

    static bool shallAdd(const std::shared_ptr<OrderBase> &order);

    void eventsReceived(const IndexedEventSource &source, const std::vector<std::shared_ptr<OrderBase>> &events,
                        bool isSnapshot);

    void notifyListeners();

    void scheduleTaskIfNeeded(std::chrono::milliseconds delay);

    void rescheduleTaskIfNeeded(std::chrono::milliseconds delay);

    bool tryCancelTask();

    bool update(const IndexedEventSource &source, const std::vector<std::shared_ptr<OrderBase>> &events,
                bool isSnapshot);

    std::vector<std::shared_ptr<OrderBase>> getBuyOrders();

    std::vector<std::shared_ptr<OrderBase>> getSellOrders();

    void clearBySource(const IndexedEventSource &source);

  public:
    MarketDepthModel(LockExternalConstructionTag, const std::shared_ptr<Builder> &builder);

    ~MarketDepthModel() override;

    static std::shared_ptr<Builder> newBuilder();

    std::size_t getDepthLimit() const;

    void setDepthLimit(std::size_t depthLimit);

    std::int64_t getAggregationPeriod() const;

    void setAggregationPeriod(std::int64_t aggregationPeriodMillis);

    void setAggregationPeriod(std::chrono::milliseconds aggregationPeriod);

    void close() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()