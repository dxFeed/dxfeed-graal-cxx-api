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
 * Type of an iceberg order.
 */
struct IcebergType : Enum<IcebergType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Iceberg type is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT IcebergType UNDEFINED;

    /**
     * Represents native (exchange-managed) iceberg type.
     */
    static const DXFCPP_EXPORT IcebergType NATIVE;

    /**
     * Represents synthetic (managed outside of the exchange) iceberg type
     */
    static const DXFCPP_EXPORT IcebergType SYNTHETIC;
};

template <>
const std::unordered_map<IcebergType::CodeType, std::reference_wrapper<const IcebergType>> IcebergType::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()