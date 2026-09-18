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

/**
 * A presentation-oriented copy of the fields used from a TimeAndSale event.
 *
 * The sample copies events into this value type before passing them from a dxFeed callback to the UI thread. This
 * keeps the snapshot accumulator independent of the feed objects and makes it straightforward to test without a
 * connection.
 */
struct TimeAndSaleRow {
    std::int64_t time{};              ///< Event time in milliseconds since the Unix epoch.
    std::int64_t index{};             ///< Unique TimeAndSale index used to replace or remove an event.
    std::int16_t exchangeCode{};      ///< Exchange code represented as an ASCII character when positive.
    double price{};                   ///< Trade price.
    double size{};                    ///< Trade size.
    double bidPrice{};                ///< Market bid price reported with the trade.
    double askPrice{};                ///< Market ask price reported with the trade.
    std::string saleConditions{};     ///< Exchange-specific sale condition codes.
    bool remove{};                    ///< Whether this row removes the event with the same index.
};

/**
 * Accumulates IndexedTxModel transactions into a bounded, presentation-ready snapshot.
 *
 * Rows are keyed by TimeAndSale index, so later events replace earlier versions and REMOVE events erase the
 * corresponding row. A snapshot transaction clears the previously accumulated state before its events are applied.
 * Only the rows with the greatest indexes are retained when the configured size limit is exceeded.
 *
 * This class is not thread-safe. The sample serializes access to it in UiMailbox.
 */
class TimeAndSalesStore final {
    std::size_t sizeLimit_;
    std::map<std::int64_t, TimeAndSaleRow, std::greater<>> rowsByIndex_{};

    void trim() {
        while (rowsByIndex_.size() > sizeLimit_) {
            rowsByIndex_.erase(std::prev(rowsByIndex_.end()));
        }
    }

    public:
    /**
     * Creates an empty accumulator.
     *
     * @param sizeLimit Maximum number of rows retained after each transaction. A zero limit discards every row.
     */
    explicit TimeAndSalesStore(std::size_t sizeLimit) : sizeLimit_(sizeLimit) {
    }

    /// Removes all accumulated rows.
    void clear() noexcept {
        rowsByIndex_.clear();
    }

    /**
     * Applies one complete transaction delivered by IndexedTxModel.
     *
     * @param events Rows belonging to the transaction. Rows marked as REMOVE erase their matching indexes.
     * @param isSnapshot Whether the transaction represents a complete snapshot and must replace existing state.
     */
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

    /// Returns the number of currently accumulated rows.
    [[nodiscard]] std::size_t size() const noexcept {
        return rowsByIndex_.size();
    }

    /**
     * Creates a detached presentation snapshot ordered from the oldest trade to the newest.
     *
     * Equal timestamps are ordered by event index, producing deterministic output that matches the table direction
     * used by the original Swing sample.
     *
     * @return A copy of all retained rows in presentation order.
     */
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
