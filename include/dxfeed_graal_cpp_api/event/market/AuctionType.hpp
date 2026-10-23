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
 * Type of auction.
 *
 * This is an experimental API, it may change in future versions.
 */
struct AuctionType : Enum<AuctionType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Type is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT AuctionType UNDEFINED;

    /**
     * Represents an opening auction.
     */
    static const DXFCPP_EXPORT AuctionType OPENING;

    /**
     * Represents a closing auction.
     */
    static const DXFCPP_EXPORT AuctionType CLOSING;

    /**
     * Represents a reopening auction, after a halt or pause and IPO.
     */
    static const DXFCPP_EXPORT AuctionType REOPENING;

    /**
     * Other auction types.
     */
    static const DXFCPP_EXPORT AuctionType OTHER;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const AuctionType>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
