// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Enum.hpp"

#include <cstdint>
#include <functional>
#include <span>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Side of an order imbalance.
 *
 * This is an experimental API, it may change in future versions.
 */
struct ImbalanceSide : Enum<ImbalanceSide, std::uint32_t> {
    using Enum::Enum;

    /**
     * Side is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT ImbalanceSide UNDEFINED;

    /**
     * Buy side (bid) imbalance.
     */
    static const DXFCPP_EXPORT ImbalanceSide BUY;

    /**
     * Sell side (ask or offer) imbalance.
     */
    static const DXFCPP_EXPORT ImbalanceSide SELL;

    /**
     * No order imbalance exists.
     */
    static const DXFCPP_EXPORT ImbalanceSide NO_IMBALANCE;

    /**
     * Insufficient orders to calculate imbalance.
     *
     * Applicable only to NASDAQ and OTC Markets exchanges.
     */
    static const DXFCPP_EXPORT ImbalanceSide INSUFFICIENT_ORDERS;

    /**
     * Imbalance calculation is paused.
     *
     * Applicable only to NASDAQ exchange.
     */
    static const DXFCPP_EXPORT ImbalanceSide PAUSED;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const ImbalanceSide>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
