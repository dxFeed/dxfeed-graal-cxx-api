// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Enum.hpp"

#include <cstdint>

/**
 * \addtogroup dxfcpp_market
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Type of iceberg order.
 */
struct IcebergType : Enum<IcebergType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Iceberg type is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT IcebergType UNDEFINED;

    /**
     * Represents a native (exchange-managed) iceberg type.
     */
    static const DXFCPP_EXPORT IcebergType NATIVE;

    /**
     * Represents synthetic (managed outside the exchange) iceberg type
     */
    static const DXFCPP_EXPORT IcebergType SYNTHETIC;

    /// All elements of the enum (constant-initialized).
    static const DXFCPP_EXPORT std::span<const std::reference_wrapper<const IcebergType>> ALL;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()