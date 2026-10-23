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
 * Type of Time in Force for the orders on the Nuam Market.
 *
 * This is an experimental API, it may change in future versions.
 *
 * Java:
 * <a href="https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/custom/NuamTimeInForceType.html">NuamTimeInForceType</a>
 */
struct NuamTimeInForceType : Enum<NuamTimeInForceType, std::uint32_t> {
    using Enum::Enum;

    /**
     * The time in force type is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType UNDEFINED;

    /**
     * The order will expire at the end of the day it was entered.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType REST_OF_DAY;

    /**
     * The order will rest on the order book until it is cancelled or fully executed.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType GTC;

    /**
     * In continuous matching the order will execute upon reception but never added
     * to the order book. In an auction period it will be added to the order book in order
     * to participate in the next uncross.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType IMMEDIATE_OR_CANCEL;

    /**
     * In continuous matching the order will execute upon reception but never added to the order book.
     * The difference compared to NuamTimeInForceType::IMMEDIATE_OR_CANCEL is this order must execute in full.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType FILL_OR_KILL;

    /**
     * The order will expire at the end of the specified session.
     * This time in force type will utilise the additional field for defining the session id.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType GOOD_TIL_SESSION;

    /**
     * The order will expire after the defined number of days where the day the order is entered is counted as one day.
     * This time in force type will utilise the additional field for defining the number of days.
     */
    static const DXFCPP_EXPORT NuamTimeInForceType NUMBER_OF_DAYS;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const NuamTimeInForceType>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
