// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/model/MarketDepthModel.hpp"
#include "dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

DXFCPP_BEGIN_NAMESPACE

MarketDepthModel::Builder::Builder(LockExternalConstructionTag) {
}

MarketDepthModel::Builder::~Builder() = default;

std::shared_ptr<MarketDepthModel::Builder> MarketDepthModel::Builder::withFeed(std::shared_ptr<DXFeed> feed) {
    builder_ = builder_->withFeed(feed);

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

int MarketDepthModel::OrderComparator::operator()(const std::shared_ptr<OrderBase> &o1,
                                                  const std::shared_ptr<OrderBase> &o2) const {
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

    if (o1->is<Order>() && o2->is<Order>()) {
        auto order1 = o1->sharedAs<Order>();
        auto order2 = o2->sharedAs<Order>();

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

int MarketDepthModel::BuyComparator::operator()(const std::shared_ptr<OrderBase> &o1,
                                                const std::shared_ptr<OrderBase> &o2) const {
    if (o1->getPrice() < o2->getPrice()) {
        return 1; // desc
    }

    if (o1->getPrice() > o2->getPrice()) {
        return -1;
    }

    return OrderComparator{}(o1, o2);
}

int MarketDepthModel::SellComparator::operator()(const std::shared_ptr<OrderBase> &o1,
                                                 const std::shared_ptr<OrderBase> &o2) const {
    if (o1->getPrice() < o2->getPrice()) {
        return 1; // desc
    }

    if (o1->getPrice() > o2->getPrice()) {
        return -1;
    }

    return OrderComparator{}(o1, o2);
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

bool MarketDepthModel::shallAdd(const std::shared_ptr<OrderBase> &order) {
    return order->hasSize() && !order->getEventFlagsMask().contains(EventFlag::REMOVE_EVENT);
}

void MarketDepthModel::eventsReceived(const IndexedEventSource &source,
                                      const std::vector<std::shared_ptr<OrderBase>> &events, bool isSnapshot) {
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

void MarketDepthModel::notifyListeners() {
    std::lock_guard guard(mtx_);

    listener_->getHandler().handle(getBuyOrders(), getSellOrders());
    taskScheduled_ = false;
}

void MarketDepthModel::scheduleTaskIfNeeded(std::chrono::milliseconds delay) {
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

void MarketDepthModel::rescheduleTaskIfNeeded(std::chrono::milliseconds delay) {
    std::lock_guard guard(mtx_);

    if (tryCancelTask() && delay.count() != 0) {
        scheduleTaskIfNeeded(delay);
    }
}

bool MarketDepthModel::tryCancelTask() {
    std::lock_guard guard(mtx_);

    if (taskScheduled_ && taskTimer_ && taskTimer_->isRunning()) {
        taskTimer_->stop();
        taskTimer_.reset();
        taskScheduled_ = false;

        return true;
    }

    return false;
}

bool MarketDepthModel::update(const IndexedEventSource &source, const std::vector<std::shared_ptr<OrderBase>> &events,
                              bool isSnapshot) {
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

std::vector<std::shared_ptr<OrderBase>> MarketDepthModel::getBuyOrders() {
    std::lock_guard guard(mtx_);

    return buyOrders_.toVector();
}

std::vector<std::shared_ptr<OrderBase>> MarketDepthModel::getSellOrders() {
    std::lock_guard guard(mtx_);

    return sellOrders_.toVector();
}

void MarketDepthModel::clearBySource(const IndexedEventSource &source) {
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

MarketDepthModel::MarketDepthModel(LockExternalConstructionTag, const std::shared_ptr<Builder> &builder) {
    depthLimit_ = builder->depthLimit_;
    buyOrders_.setDepthLimit(depthLimit_);
    sellOrders_.setDepthLimit(depthLimit_);
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

    if (depthLimit == depthLimit_) {
        return;
    }

    depthLimit_ = depthLimit;
    buyOrders_.setDepthLimit(depthLimit);
    sellOrders_.setDepthLimit(depthLimit);
    tryCancelTask();
    notifyListeners();
}

std::int64_t MarketDepthModel::getAggregationPeriod() const {
    std::lock_guard guard(mtx_);

    return aggregationPeriodMillis_;
}

void MarketDepthModel::setAggregationPeriod(std::int64_t aggregationPeriodMillis) {
    std::lock_guard guard(mtx_);

    if (aggregationPeriodMillis == aggregationPeriodMillis_) {
        return;
    }

    aggregationPeriodMillis_ = aggregationPeriodMillis;
    rescheduleTaskIfNeeded(std::chrono::milliseconds(aggregationPeriodMillis_));
}

void MarketDepthModel::setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
    setAggregationPeriod(aggregationPeriod.count());
}

void MarketDepthModel::close() const {
    std::lock_guard guard(mtx_);

    indexedTxModel_->close();
}

DXFCPP_END_NAMESPACE