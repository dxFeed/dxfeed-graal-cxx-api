// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "OrderBook.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

/**
 * This class manages multiple market depth models for different symbols.
 * It provides methods to add and remove symbols and to retrieve the current order book for a symbol.
 *
 * @tparam O The type of order.
 */
template <dxfcpp::Derived<dxfcpp::OrderBase> O>
struct MultipleMarketDepthModel : dxfcpp::RequireMakeShared<MultipleMarketDepthModel<O>> {
    using ListenerSignature = void(const dxfcpp::IndexedEventSubscriptionSymbol &, std::shared_ptr<OrderBook<O>>);

    /// A builder class for creating instances of MultipleMarketDepthModel.
    struct Builder : dxfcpp::RequireMakeShared<Builder> {
        friend struct MultipleMarketDepthModel;

      private:
        std::shared_ptr<typename dxfcpp::MarketDepthModel<O>::Builder> builder_{};

        std::function<ListenerSignature> listener_{};

      public:
        Builder(dxfcpp::RequireMakeShared<Builder>::LockExternalConstructionTag) {
            builder_ = dxfcpp::MarketDepthModel<O>::newBuilder();
        }

        ~Builder() override {
        }

        /**
         * Sets the feed for the market depth model.
         *
         * @param feed The feed to use.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFeed(std::shared_ptr<dxfcpp::DXFeed> feed) {
            builder_ = builder_->withFeed(std::move(feed));

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the depth limit for the market depth model.
         *
         * @param depthLimit The depth limit.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withDepthLimit(std::size_t depthLimit) {
            builder_ = builder_->withDepthLimit(depthLimit);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the aggregation period for the market depth model.
         *
         * @param aggregationPeriodMillis The aggregation period in ms.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::int64_t aggregationPeriodMillis) {
            builder_ = builder_->withAggregationPeriod(aggregationPeriodMillis);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the aggregation period for the market depth model.
         *
         * @param aggregationPeriod The aggregation period.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
            builder_ = builder_->withAggregationPeriod(aggregationPeriod);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the listener for updates to the order book.
         *
         * @param onEventsReceived The listener.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withListener(std::function<ListenerSignature> onEventsReceived) {
            listener_ = onEventsReceived;

            return this->template sharedAs<Builder>();
        }

        /**
         * Builds and returns an instance of MultipleMarketDepthModel.
         *
         * @return A new instance of MultipleMarketDepthModel.
         */
        std::shared_ptr<MultipleMarketDepthModel> build() {
            return MultipleMarketDepthModel::createShared(this->template sharedAs<Builder>());
        }
    };

  private:
    mutable std::recursive_mutex mutex{};

    std::unordered_map<dxfcpp::IndexedEventSubscriptionSymbol, std::shared_ptr<dxfcpp::MarketDepthModel<O>>> models_{};
    std::unordered_map<dxfcpp::IndexedEventSubscriptionSymbol, std::shared_ptr<OrderBook<O>>> books_{};
    std::shared_ptr<typename dxfcpp::MarketDepthModel<O>::Builder> builder_{};

    std::function<ListenerSignature> listener_{};

    void onBook(const dxfcpp::IndexedEventSubscriptionSymbol &symbol, const std::vector<std::shared_ptr<O>> &buy,
                const std::vector<std::shared_ptr<O>> &sell) {
        std::lock_guard lock(mutex);
        std::shared_ptr<OrderBook<O>> book{};

        if (!models_.contains(symbol)) {
            return;
        }

        if (auto bookIt = books_.find(symbol); bookIt != books_.end()) {
            book = bookIt->second;
            bookIt->second->buy = buy;
            bookIt->second->sell = sell;
        } else {
            book = std::make_shared<OrderBook<O>>(buy, sell);
            books_[symbol] = book;
        }

        listener_(symbol, book);
    }

    std::shared_ptr<dxfcpp::MarketDepthModel<O>> createModel(const dxfcpp::IndexedEventSubscriptionSymbol &symbol) {
        return builder_->withSymbol(*symbol.getEventSymbol())
            ->withSources({*symbol.getSource()})
            ->withListener([self = this->weak_from_this(), symbol](const std::vector<std::shared_ptr<O>> &buy,
                                                                   const std::vector<std::shared_ptr<O>> &sell) {
                if (const auto model = self.lock()) {
                    model->template sharedAs<MultipleMarketDepthModel>()->onBook(symbol, buy, sell);
                }
            })
            ->build();
    }

  public:
    MultipleMarketDepthModel(typename dxfcpp::RequireMakeShared<MultipleMarketDepthModel>::LockExternalConstructionTag,
                             const std::shared_ptr<Builder> &builder) {
        builder_ = builder->builder_;
        listener_ = builder->listener_;
    }

    /**
     * @return A new builder.
     */
    static std::shared_ptr<Builder> newBuilder() {
        return Builder::createShared();
    }

    /**
     * Adds a symbol to the model.
     *
     * @param symbol The symbol.
     */
    void addSymbol(const dxfcpp::IndexedEventSubscriptionSymbol &symbol) {
        std::lock_guard lock(mutex);

        if (models_.contains(symbol)) {
            return;
        }

        models_[symbol] = createModel(symbol);
    }

    /**
     * Removes a symbol from the model.
     *
     * @param symbol The symbol.
     */
    void removeSymbol(const dxfcpp::IndexedEventSubscriptionSymbol &symbol) {
        std::lock_guard lock(mutex);

        if (auto it = models_.find(symbol); it != models_.end()) {
            it->second->close();
            models_.erase(symbol);
            books_.erase(symbol);
        }
    }

    /**
     * Tries to get the order book for a given symbol.
     *
     * @param symbol The symbol.
     * @return return a pair `{true, std::shared_ptr<OrderBook<O>>}`; otherwise, `{false, nullptr}`.
     */
    std::pair<bool, std::shared_ptr<OrderBook<O>>> tryGetBook(const dxfcpp::IndexedEventSubscriptionSymbol &symbol) {
        std::lock_guard lock(mutex);

        if (books_.contains(symbol)) {
            return {true, books_[symbol]};
        }

        return {false, nullptr};
    }
};
