// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>

/// Represents a price level in a price level book.
struct PriceLevel {
  private:
    std::optional<std::string> eventSymbol_;
    const dxfcpp::IndexedEventSource &eventSource_;
    const dxfcpp::Side &side_;
    double price_;
    double size_;

  public:
    /**
     * Initializes a new default instance.
     */
    PriceLevel() : eventSource_{dxfcpp::IndexedEventSource::DEFAULT}, side_{dxfcpp::Side::UNDEFINED}, price_{}, size_{} {
    }

    /**
     * Initializes a new instance by copying an existing price level.
     *
     * @param pl The price level to copy.
     */
    PriceLevel(const PriceLevel &pl)
        : eventSymbol_{pl.eventSymbol_}, eventSource_{pl.eventSource_}, side_{pl.side_}, price_{pl.price_},
          size_(pl.size_) {
    }

    /**
     * Initializes a new instance using an order.
     *
     * @param order The order to initialize the price level from.
     */
    explicit PriceLevel(const std::shared_ptr<dxfcpp::OrderBase> &order)
        : eventSymbol_{order->getEventSymbolOpt()}, eventSource_{order->getSource()}, side_{order->getOrderSide()},
          price_{order->getPrice()}, size_{order->getSize()} {
    }

    /// Returns the optional event symbol.
    const std::optional<std::string> &getEventSymbol() const {
        return eventSymbol_;
    }

    /// Returns the event source.
    const dxfcpp::IndexedEventSource &getEventSource() const {
        return eventSource_;
    }

    /// Returns the side of the price level.
    const dxfcpp::Side &getSide() const {
        return side_;
    }

    /// Returns the price of the price level.
    double getPrice() const {
        return price_;
    }

    /// Returns the size of the price level.
    double getSize() const {
        return size_;
    }

    /**
     * Sets the size of the price level.
     *
     * @param size The new price level size.
     */
    void setSize(const double size) {
        size_ = size;
    }

    /**
     * Compares two price levels without taking price into account.
     *
     * @param pl1 The first price level to compare.
     * @param pl2 The second price level to compare.
     * @return Returns `true` if the event symbols are not nullptr, are equal, and the sides are equal.
     */
    friend bool operator==(const std::shared_ptr<PriceLevel> &pl1, const std::shared_ptr<PriceLevel> &pl2) {
        if (pl1->getEventSymbol() == std::nullopt || pl2->getEventSymbol() == std::nullopt) {
            return false;
        }

        if (pl1->getEventSymbol() != pl2->getEventSymbol()) {
            return false;
        }

        return pl1->getSide() == pl2->getSide();
    }

    /// Returns a string representation of this price level.
    std::string toString() const {
        return "PriceLevel{" + eventSymbol_.value_or(dxfcpp::String::NUL) + ", source=" + eventSource_.toString() +
               ", side=" + side_.toString() + ", price=" + dxfcpp::toString(price_) +
               ", size=" + dxfcpp::toString(size_) + "}";
    }
};
