// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/market/OrderBase.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "IndexedTxModel.hpp"
#include "MarketDepthModelListener.hpp"

#include <memory>
#include <unordered_set>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;
struct SymbolWrapper;

struct DXFCPP_EXPORT MarketDepthModel : RequireMakeShared<MarketDepthModel> {

    struct DXFCPP_EXPORT Builder : RequireMakeShared<Builder> {
        std::shared_ptr<IndexedTxModel::Builder> builder;
        std::shared_ptr<MarketDepthModelListenerCommon> listener;
        std::size_t depthLimit;
        std::int64_t aggregationPeriodMillis;

        std::shared_ptr<Builder> withFeed(std::shared_ptr<DXFeed> feed) {
            builder = builder->withFeed(std::move(feed));

            return sharedAs<Builder>();
        }

        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol) {
            builder = builder->withSymbol(symbol);

            return sharedAs<Builder>();
        }

        template <Derived<OrderBase> E>
        std::shared_ptr<Builder> withListener(std::shared_ptr<MarketDepthModelListener<E>> listener) {
            listener = listener->template sharedAs<MarketDepthModelListenerCommon>();

            return sharedAs<Builder>();
        }

        template <typename EventSourceIt>
        std::shared_ptr<Builder> withSources(EventSourceIt begin, EventSourceIt end) const {
            builder = builder->withSources(begin, end);

            return sharedAs<Builder>();
        }

        template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
        std::shared_ptr<Builder> withSources(EventSourceCollection &&sources) const {
            return withSources(std::begin(sources), std::end(sources));
        }

        std::shared_ptr<Builder> withSources(std::initializer_list<EventSourceWrapper> sources) const;

        std::shared_ptr<Builder> withDepthLimit(std::size_t depthLimit) const;

        std::shared_ptr<Builder> withAggregationPeriod(std::int64_t aggregationPeriodMillis) const;

        std::shared_ptr<Builder> withAggregationPeriod(std::chrono::milliseconds aggregationPeriod) const;

        std::shared_ptr<MarketDepthModel> build() const;
    };

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