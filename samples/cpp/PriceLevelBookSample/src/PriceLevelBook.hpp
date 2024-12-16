// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "PriceLevel.hpp"

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

template <dxfcpp::Derived<dxfcpp::OrderBase> O> struct PriceLevelBook {
    struct BuyLess {
        bool operator()(const std::shared_ptr<PriceLevel> &pl1, const std::shared_ptr<PriceLevel> &pl2) const {
            return pl1->getPrice() < pl2->getPrice();
        }
    };

    struct SellLess {
        bool operator()(const std::shared_ptr<PriceLevel> &pl1, const std::shared_ptr<PriceLevel> &pl2) const {
            return pl1->getPrice() > pl2->getPrice();
        }
    };

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
        void setDepthLimit(std::size_t depthLimit) {
            std::lock_guard lock(mutex_);

            if (depthLimit_ == depthLimit) {
                return;
            }

            depthLimit_ = depthLimit;
            isChanged_ = true;
        }

        std::size_t getDepthLimit() const {
            return depthLimit_;
        }

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

        void eraseOrDecrease(const std::shared_ptr<PriceLevel> &pl) {
            std::lock_guard lock(mutex_);

            auto it = priceLevels_.find(pl);

            if (it != priceLevels_.end()) {
                auto existing = *it;
                auto newSize = existing->getSize() - pl->getSize();

                priceLevels_.erase(it);

                if (!dxfcpp::math::equals(newSize, 0.0) && newSize > 0.0) {
                    existing->setSize(newSize);
                    priceLevels_.emplace(std::make_shared<PriceLevel>(*existing));
                }

                markAsChangedIfNeeded(existing);
            }
        }

        void clearBySource(const dxfcpp::IndexedEventSource& source) {
            std::lock_guard lock(mutex_);

            std::size_t size = priceLevels_.size();

            for (auto it = priceLevels_.begin(); it != priceLevels_.end(); ++it) {
                if ((*it)->getSource() == source) {
                    it = priceLevels_.erase(it);
                }
            }

            return priceLevels_.size() != size;
        }

        std::vector<std::shared_ptr<PriceLevel>> toVector() {
            std::lock_guard lock(mutex_);

            if (isChanged_) {
                updateSnapshot();
            }

            return {snapshot_.begin(), snapshot_.end()};
        }
    };

    mutable std::recursive_mutex mtx{};
    std::unordered_map<std::int64_t, std::shared_ptr<O>> ordersByIndex_{};
};
