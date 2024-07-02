// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

/**
 * Action enum for the Full Order Book (FOB) Orders. Action describes business meaning of the Order event:
 * whether order was added or replaced, partially or fully executed, etc.
 */
struct OrderAction : Enum<OrderAction, std::uint32_t> {
    using Enum::Enum;

    /**
     * Default enum value for orders that do not support "Full Order Book" and for backward compatibility -
     * action must be derived from other Order fields.
     *
     * <p>All Full Order Book related fields for this action will be empty.
     */
    static const DXFCPP_EXPORT OrderAction UNDEFINED;

    /**
     * New Order is added to Order Book.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>@ref Order::getAuxOrderId() "auxOrderId" - ID of the order replaced by this new order - if available.</li>
     * <li>Trade fields will be empty</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction NEW;

    /**
     * Order is modified and price-time-priority is not maintained (i.e. order has re-entered Order Book).
     * Order @ref Order::getEventSymbol() "symbol" and @ref Order::getOrderSide() "side" will remain the same.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>Trade fields will be empty</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction REPLACE;

    /**
     * Order is modified without changing its price-time-priority (usually due to partial cancel by user).
     * Order's @ref Order::getSize() "size" will contain new updated size.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>Trade fields will be empty</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction MODIFY;

    /**
     * Order is fully canceled and removed from Order Book.
     * Order's @ref Order::getSize() "size" will be equal to 0.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>@ref Order::getAuxOrderId() "auxOrderId" - ID of the new order replacing this order - if available.</li>
     * <li>Trade fields will be empty</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction DELETE;

    /**
     * Size is changed (usually reduced) due to partial order execution.
     * Order's @ref Order::getSize() "size" will be updated to show current outstanding size.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>@ref Order::getAuxOrderId() "auxOrderId" - aggressor order ID, if available</li>
     * <li>@ref Order::getTradeId() "tradeId" - if available</li>
     * <li>@ref Order::getTradeSize() "tradeSize" and @ref Order::getTradePrice() "tradePrice" -
     * contain size and price of this execution</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction PARTIAL;

    /**
     * Order is fully executed and removed from Order Book.
     * Order's {@ref Order#getSize() size} will be equals to 0.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always present</li>
     * <li>@ref Order::getAuxOrderId() "auxOrderId" - aggressor order ID, if available</li>
     * <li>@ref Order::getTradeId() "tradeId" - if available</li>
     * <li>@ref Order::getTradeSize() "tradeSize" and @ref Order::getTradePrice() "tradePrice" -
     * contain size and price of this execution - always present</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction EXECUTE;

    /**
     * Non-Book Trade - this Trade not refers to any entry in Order Book.
     * Order's @ref Order::getSize() "size" and @ref Order::getPrice() "price" will be equals to 0.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always empty</li>
     * <li>@ref Order::getTradeId() "tradeId" - if available</li>
     * <li>@ref Order::getTradeSize() "tradeSize" and @ref Order::getTradePrice() "tradePrice" -
     * contain size and price of this trade - always present</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction TRADE;

    /**
     * Prior Trade/Order Execution bust.
     * Order's @ref Order::getSize() "size" and @ref Order::getPrice() "price" will be equals to 0.
     *
     * <p>Full Order Book fields:
     * <ul>
     * <li>@ref Order::getOrderId() "orderId" - always empty</li>
     * <li>@ref Order::getTradeId() "tradeId" - always present</li>
     * <li>@ref Order::getTradeSize() "tradeSize" and @ref Order::getTradePrice() "tradePrice" - always empty</li>
     * </ul>
     */
    static const DXFCPP_EXPORT OrderAction BUST;
};

template <>
const std::unordered_map<OrderAction::CodeType, std::reference_wrapper<const OrderAction>> OrderAction::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()