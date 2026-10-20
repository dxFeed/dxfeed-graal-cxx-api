// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Enum.hpp"

#include <cstdint>
#include <type_traits>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Short sale restriction on an instrument.
 */
struct ShortSaleRestriction : Enum<ShortSaleRestriction, std::uint32_t> {
    using Enum::Enum;

    /**
     * Short sale restriction is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT ShortSaleRestriction UNDEFINED;

    /**
     * Short sale restriction is active.
     */
    static const DXFCPP_EXPORT ShortSaleRestriction ACTIVE;

    /**
     * Short sale restriction is inactive.
     */
    static const DXFCPP_EXPORT ShortSaleRestriction INACTIVE;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const ShortSaleRestriction>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()