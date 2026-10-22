// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/orcs/PriceLevelChecker.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/orcs/IsolatedPriceLevelService.hpp"

DXFCPP_BEGIN_NAMESPACE

bool PriceLevelChecker::validate(const std::vector<std::shared_ptr<Order>> &orders,
                                 std::chrono::milliseconds timeGapBound, bool printQuotes) {
    for (const auto &order : orders) {
        if (!order) {
            throw InvalidArgumentException("PriceLevelChecker::validate: the list of orders contains a null pointer");
        }
    }

    return isolated::orcs::IsolatedPriceLevelChecker::validate(orders, static_cast<std::int64_t>(timeGapBound.count()),
                                                               printQuotes);
}

DXFCPP_END_NAMESPACE
