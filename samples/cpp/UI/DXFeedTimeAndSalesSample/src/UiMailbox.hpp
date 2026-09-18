// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "TimeAndSalesStore.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace dxfeed::time_and_sales_sample {

/** A detached copy of all data required to render one UI frame. */
struct ViewState {
    std::string symbol{};                 ///< Currently subscribed TimeAndSale symbol.
    std::string description{};            ///< Latest instrument description, or an empty string if unavailable.
    std::vector<TimeAndSaleRow> rows{};    ///< Accumulated trades in presentation order.
};

/**
 * Synchronizes dxFeed callback data with the UI thread.
 *
 * Feed callbacks publish transactions and profile updates from dxFeed-managed threads. The render loop periodically
 * takes a detached ViewState when anything has changed. Updates are coalesced into the latest state rather than queued,
 * which keeps feed callbacks independent of the GLFW event loop and avoids requiring a custom dxFeed executor.
 */
class UiMailbox final {
    mutable std::mutex mutex_{};
    TimeAndSalesStore store_;
    std::string symbol_{};
    std::string profileSymbol_{};
    std::string description_{};
    std::uint64_t generation_{};
    bool dirty_{true};

    public:
    /**
     * Creates an empty mailbox.
     *
     * @param sizeLimit Maximum number of TimeAndSale rows retained in the view.
     */
    explicit UiMailbox(std::size_t sizeLimit) : store_(sizeLimit) {
    }

    /**
     * Starts a new subscription generation and clears trades belonging to the previous symbol.
     *
     * The description is retained when the base Profile symbol does not change. For example, switching from AAPL to
     * AAPL&Q keeps the already received AAPL profile because DXFeedSubscription::setSymbols does not redeliver an
     * event when its symbol set remains unchanged.
     *
     * @param symbol New normalized TimeAndSale symbol, or an empty string when unsubscribing.
     * @param profileSymbol Base symbol used by the Profile subscription.
     * @return Generation token that the corresponding IndexedTxModel listener must use when publishing transactions.
     */
    std::uint64_t reset(std::string symbol, std::string profileSymbol) {
        const std::lock_guard lock{mutex_};

        ++generation_;
        symbol_ = std::move(symbol);

        if (profileSymbol_ != profileSymbol) {
            description_.clear();
        }

        profileSymbol_ = std::move(profileSymbol);
        store_.clear();
        dirty_ = true;

        return generation_;
    }

    /**
     * Applies a TimeAndSale transaction received from IndexedTxModel.
     *
     * Transactions from an already closed model may arrive after a symbol change. Such transactions are ignored when
     * their generation token no longer matches the active subscription.
     *
     * @param generation Subscription generation captured when the listener was created.
     * @param events Rows belonging to the transaction.
     * @param isSnapshot Whether the transaction replaces the previously accumulated snapshot.
     */
    void publishTrades(std::uint64_t generation, const std::vector<TimeAndSaleRow> &events, bool isSnapshot) {
        const std::lock_guard lock{mutex_};

        if (generation != generation_) {
            return;
        }

        store_.apply(events, isSnapshot);
        dirty_ = true;
    }

    /**
     * Publishes an instrument description received from the Profile subscription.
     *
     * @param symbol Symbol to which the profile belongs. Profiles for inactive symbols are ignored.
     * @param description Optional instrument description.
     */
    void publishProfile(const std::string &symbol, const std::optional<std::string> &description) {
        const std::lock_guard lock{mutex_};

        if (symbol != profileSymbol_) {
            return;
        }

        description_ = description.value_or("");
        dirty_ = true;
    }

    /**
     * Takes the latest detached view when state has changed since the previous call.
     *
     * Taking a view clears the dirty flag. A later callback sets it again, causing another view to be produced.
     *
     * @return The latest view, or std::nullopt when the UI already has the current state.
     */
    [[nodiscard]] std::optional<ViewState> takeIfDirty() {
        const std::lock_guard lock{mutex_};

        if (!dirty_) {
            return std::nullopt;
        }

        dirty_ = false;

        return ViewState{symbol_, description_, store_.snapshot()};
    }
};

} // namespace dxfeed::time_and_sales_sample
