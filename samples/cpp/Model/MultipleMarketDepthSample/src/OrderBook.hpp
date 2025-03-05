// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <vector>

template <dxfcpp::Derived<dxfcpp::OrderBase> O> struct OrderBook : std::enable_shared_from_this<OrderBook<O>> {
    std::vector<std::shared_ptr<O>> buy{};
    std::vector<std::shared_ptr<O>> sell{};

    OrderBook(const std::vector<std::shared_ptr<O>> &buy, const std::vector<std::shared_ptr<O>> &sell) {
        this->buy = buy;
        this->sell = sell;
    }
};