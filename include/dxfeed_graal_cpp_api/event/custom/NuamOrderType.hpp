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
 * \addtogroup dxfcpp_custom
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Type of orders on the Nuam Market.
 *
 * This is an experimental API, it may change in future versions.
 *
 * Java: <a href="https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/custom/NuamOrderType.html">NuamOrderType</a>
 */
struct NuamOrderType : Enum<NuamOrderType, std::uint32_t> {
    using Enum::Enum;

    /**
     * The order type is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT NuamOrderType UNDEFINED;

    /**
     * The order must execute within a limit price.
     */
    static const DXFCPP_EXPORT NuamOrderType LIMIT;

    /**
     * The order can execute at any price.
     */
    static const DXFCPP_EXPORT NuamOrderType MARKET;

    /**
     * The order is first executed as a market order. If it cannot be fully executed,
     * any remaining quantity is then converted into a limit order, with the execution price serving as the limit.
     */
    static const DXFCPP_EXPORT NuamOrderType MARKET_TO_LIMIT;

    /**
     * The order is entered without a price and if there is a best price on the same side of the order book (BBO),
     * the order will be given this price and added to the order book.
     */
    static const DXFCPP_EXPORT NuamOrderType BEST_ORDER;

    /**
     * Imbalance orders only participate in auctions and only of there is a surplus on the same side
     * as the imbalance order. Imbalance orders are not part of the EP calculation.
     */
    static const DXFCPP_EXPORT NuamOrderType IMBALANCE;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const NuamOrderType>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
