// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
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

        std::shared_ptr<MarketDepthModel> build() const;
    };

  private:
    std::recursive_mutex mtx_{};
    std::shared_ptr<IndexedTxModel> indexedTxModel_{};
    std::shared_ptr<MarketDepthModelListenerCommon> listener_{};
    std::size_t depthLimit_{};
    std::int64_t aggregationPeriodMillis_{};
    std::shared_ptr<Timer> timer_{};

    static std::shared_ptr<MarketDepthModel> create(std::shared_ptr<Builder> builder);

    void eventsReceived(const IndexedEventSource &source, const std::vector<std::shared_ptr<OrderBase>> &events,
                        bool isSnapshot);

    void notifyListeners();

  public:
    MarketDepthModel(LockExternalConstructionTag, const std::shared_ptr<Builder> &builder);

    ~MarketDepthModel() override;

    static std::shared_ptr<Builder> newBuilder();

    std::size_t getDepthLimit() const;

    void setDepthLimit(std::size_t depthLimit);

    std::int64_t getAggregationPeriod() const;

    void setAggregationPeriod(std::int64_t aggregationPeriodMillis);

    void setAggregationPeriod(std::chrono::milliseconds aggregationPeriod);

    void close();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()