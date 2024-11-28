// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/model/MarketDepthModel.hpp"
#include "dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

DXFCPP_BEGIN_NAMESPACE

MarketDepthModel::Builder::Builder(RequireMakeShared<Builder>::LockExternalConstructionTag) {
}

MarketDepthModel::Builder::~Builder() = default;

std::shared_ptr<MarketDepthModel::Builder> MarketDepthModel::Builder::withFeed(std::shared_ptr<DXFeed> feed) {
    builder_ = builder_->withFeed(std::move(feed));

    return sharedAs<Builder>();
}

std::shared_ptr<MarketDepthModel::Builder> MarketDepthModel::Builder::withSymbol(const SymbolWrapper &symbol) {
    builder_ = builder_->withSymbol(symbol);

    return sharedAs<Builder>();
}

std::shared_ptr<MarketDepthModel::Builder>
MarketDepthModel::Builder::withSources(std::initializer_list<EventSourceWrapper> sources) {
    return withSources(sources.begin(), sources.end());
}

std::shared_ptr<MarketDepthModel::Builder> MarketDepthModel::Builder::withDepthLimit(std::size_t depthLimit) {
    depthLimit_ = depthLimit;

    return sharedAs<Builder>();
}

std::shared_ptr<MarketDepthModel::Builder>
MarketDepthModel::Builder::withAggregationPeriod(std::int64_t aggregationPeriodMillis) {
    aggregationPeriodMillis_ = aggregationPeriodMillis;

    return sharedAs<Builder>();
}

std::shared_ptr<MarketDepthModel::Builder>
MarketDepthModel::Builder::withAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
    return withAggregationPeriod(aggregationPeriod.count());
}

std::shared_ptr<MarketDepthModel> MarketDepthModel::Builder::build() const {
    return MarketDepthModel::createShared(sharedAs<Builder>());
}

MarketDepthModel::MarketDepthModel(LockExternalConstructionTag, std::shared_ptr<Builder> builder) {
    depthLimit_ = builder->depthLimit_;
    // buyOrders_.setDepthLimit(depthLimit_);
    // sellOrders.setDepthLimit(depthLimit_);
    aggregationPeriodMillis_ = builder->aggregationPeriodMillis_;
    listener_ = builder->listener_;
    indexedTxModel_ =
}

DXFCPP_END_NAMESPACE