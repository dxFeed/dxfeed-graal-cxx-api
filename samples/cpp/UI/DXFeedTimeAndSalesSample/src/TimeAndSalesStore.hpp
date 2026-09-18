// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace dxfeed::time_and_sales_sample {

struct TimeAndSaleRow {
    std::int64_t time{};
    std::int64_t index{};
    std::int16_t exchangeCode{};
    double price{};
    double size{};
    double bidPrice{};
    double askPrice{};
    std::string saleConditions{};
    bool remove{};
};

/// Accumulates IndexedTxModel transactions into a bounded, presentation-ready snapshot.
class TimeAndSalesStore final {
    std::size_t sizeLimit_;
    std::map<std::int64_t, TimeAndSaleRow, std::greater<>> rowsByIndex_{};

    void trim() {
        while (rowsByIndex_.size() > sizeLimit_) {
            rowsByIndex_.erase(std::prev(rowsByIndex_.end()));
        }
    }

    public:
    explicit TimeAndSalesStore(std::size_t sizeLimit) : sizeLimit_(sizeLimit) {
    }

    void clear() noexcept {
        rowsByIndex_.clear();
    }

    void apply(const std::vector<TimeAndSaleRow> &events, bool isSnapshot) {
        if (isSnapshot) {
            rowsByIndex_.clear();
        }

        for (const auto &event : events) {
            if (event.remove) {
                rowsByIndex_.erase(event.index);
            } else {
                rowsByIndex_.insert_or_assign(event.index, event);
            }
        }

        trim();
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return rowsByIndex_.size();
    }

    /// Returns rows from the oldest to the newest, matching the original Swing sample.
    [[nodiscard]] std::vector<TimeAndSaleRow> snapshot() const {
        std::vector<TimeAndSaleRow> result;
        result.reserve(rowsByIndex_.size());

        for (const auto &[index, row] : std::views::reverse(rowsByIndex_)) {
            result.push_back(row);
        }

        std::ranges::stable_sort(result, {}, [](const auto &row) {
            return std::pair{row.time, row.index};
        });

        return result;
    }
};

} // namespace dxfeed::time_and_sales_sample
