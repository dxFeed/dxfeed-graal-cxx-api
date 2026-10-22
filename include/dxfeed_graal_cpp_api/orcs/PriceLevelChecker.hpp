// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../event/market/Order.hpp"

#include <chrono>
#include <memory>
#include <vector>

/**
 * \addtogroup dxfcpp_orcs
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Checks the consistency of a list of price levels (Order events) received from a PriceLevelService. The list is not
 * valid when:
 * <ul>
 * <li>the orders of one time cross the opposite side of the book (the bid price is not lower than the ask price);
 * the orders of a time are checked when the orders of the next time begin, so the orders of the last time are not
 * checked;</li>
 * <li>the longest gap between subsequent orders is longer than the gap bound.</li>
 * </ul>
 * The check logs the crosses and the gaps. With `printQuotes` it also logs the bid and ask with the minimal period of
 * 1 second and the spikes in them.
 */
struct DXFCPP_EXPORT PriceLevelChecker final {
    /**
     * Checks the consistency of the list of orders.
     *
     * @param orders The list of orders sorted by time (as PriceLevelService::getOrders() returns them).
     * @param timeGapBound The bound of the gap between subsequent orders.
     * @param printQuotes `true` to log the bid and ask and their spikes during the check.
     * @return `true` if the orders are valid.
     */
    static bool validate(const std::vector<std::shared_ptr<Order>> &orders, std::chrono::milliseconds timeGapBound,
                         bool printQuotes);
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
