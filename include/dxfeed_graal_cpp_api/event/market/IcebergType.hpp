// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

#include <cstdint>

namespace dxfcpp {

/**
 * Type of an iceberg order.
 */
struct DXFCPP_EXPORT IcebergType : Enum<IcebergType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Iceberg type is undefined, unknown or inapplicable.
     */
    static const IcebergType UNDEFINED;

    /**
     * Represents native (exchange-managed) iceberg type.
     */
    static const IcebergType NATIVE;

    /**
     * Represents synthetic (managed outside of the exchange) iceberg type
     */
    static const IcebergType SYNTHETIC;
};

template <>
const std::unordered_map<IcebergType::CodeType, std::reference_wrapper<const IcebergType>> IcebergType::ParentType::ALL;

} // namespace dxfcpp