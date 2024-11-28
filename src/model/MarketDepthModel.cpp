// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/model/MarketDepthModel.hpp"
#include "dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

DXFCPP_BEGIN_NAMESPACE

MarketDepthModel::Builder::Builder(LockExternalConstructionTag) {
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

std::shared_ptr<MarketDepthModel> MarketDepthModel::Builder::build() {
    return MarketDepthModel::createShared(sharedAs<Builder>());
}

std::shared_ptr<MarketDepthModel> MarketDepthModel::create(std::shared_ptr<Builder> builder) {
    auto marketDepthModel = createShared(builder);

    marketDepthModel->indexedTxModel_ =
        builder->builder_
            ->withListener<OrderBase>([m = marketDepthModel->weak_from_this()](
                                          const IndexedEventSource &source,
                                          const std::vector<std::shared_ptr<OrderBase>> &events, bool isSnapshot) {
                if (const auto model = m.lock()) {
                    model->sharedAs<MarketDepthModel>()->eventsReceived(source, events, isSnapshot);
                }
            })
            ->build();

    return marketDepthModel;
}

void MarketDepthModel::eventsReceived(const IndexedEventSource &source,
                                      const std::vector<std::shared_ptr<OrderBase>> &events, bool isSnapshot) {
    std::lock_guard guard(mtx_);
}

void MarketDepthModel::notifyListeners() {
    //TODO: implement
    listener_->getHandler().handle({}, {});
}

MarketDepthModel::MarketDepthModel(LockExternalConstructionTag, const std::shared_ptr<Builder> &builder) {
    depthLimit_ = builder->depthLimit_;
    // buyOrders_.setDepthLimit(depthLimit_);
    // sellOrders.setDepthLimit(depthLimit_);
    aggregationPeriodMillis_ = builder->aggregationPeriodMillis_;
    listener_ = builder->listener_;
}

MarketDepthModel::~MarketDepthModel() {
    close();
}

std::shared_ptr<MarketDepthModel::Builder> MarketDepthModel::newBuilder() {
    return Builder::createShared();
}

std::size_t MarketDepthModel::getDepthLimit() const {
    std::lock_guard guard(mtx_);

    return depthLimit_;
}

void MarketDepthModel::setDepthLimit(std::size_t depthLimit) {
    std::lock_guard guard(mtx_);

    depthLimit_ = depthLimit;
}

std::int64_t MarketDepthModel::getAggregationPeriod() const {
    std::lock_guard guard(mtx_);

    return aggregationPeriodMillis_;
}

void MarketDepthModel::setAggregationPeriod(std::int64_t aggregationPeriodMillis) {
    std::lock_guard guard(mtx_);

    aggregationPeriodMillis_ = aggregationPeriodMillis;
}

void MarketDepthModel::setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
    setAggregationPeriod(aggregationPeriod.count());
}

void MarketDepthModel::close() {
    std::lock_guard guard(mtx_);

    indexedTxModel_->close();
}

DXFCPP_END_NAMESPACE