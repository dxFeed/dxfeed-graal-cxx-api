// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Enum.hpp"

#include <cstdint>
#include <type_traits>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Type of a time and sale event.
 */
struct TimeAndSaleType : Enum<TimeAndSaleType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Represents new time and sale event.
     */
    static const DXFCPP_EXPORT TimeAndSaleType NEW;

    /**
     * Represents correction time and sale event.
     */
    static const DXFCPP_EXPORT TimeAndSaleType CORRECTION;

    /**
     * Represents cancel time and sale event.
     */
    static const DXFCPP_EXPORT TimeAndSaleType CANCEL;
};

template <>
const std::unordered_map<TimeAndSaleType::CodeType, std::reference_wrapper<const TimeAndSaleType>>
    TimeAndSaleType::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()