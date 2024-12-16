// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>

struct PriceLevel {
private:
    std::optional<std::string> eventSymbol;
    const dxfcpp::IndexedEventSource &eventSource;
    const dxfcpp::Side &side;
    double price;
    double size;

public:

    PriceLevel() : eventSource{}, side{dxfcpp::Side::UNDEFINED}, price{}, size() {
    }

    PriceLevel(const PriceLevel &pl) : eventSource{pl.eventSource}, side{pl.side}, price{pl.price}, size(pl.size) {
    }

    explicit PriceLevel(const std::shared_ptr<dxfcpp::OrderBase> &order)
        : eventSymbol{order->getEventSymbolOpt()}, eventSource{order->getSource()}, side{order->getOrderSide()},
          price{order->getPrice()}, size{order->getSize()} {
    }

    const std::optional<std::string> &getEventSymbol() const {
        return eventSymbol;
    }

    const dxfcpp::IndexedEventSource &getEventSource() const {
        return eventSource;
    }

    const dxfcpp::Side &getSide() const {
        return side;
    }

    double getPrice() const {
        return price;
    }

    double getSize() const {
        return size;
    }

    void setSize(const double Size) {
        size = Size;
    }
};
