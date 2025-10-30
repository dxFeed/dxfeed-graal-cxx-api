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
 * Trading status of an instrument.
 */
struct TradingStatus : Enum<TradingStatus, std::uint32_t> {
    using Enum::Enum;

    /**
     * Trading status is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT TradingStatus UNDEFINED;

    /**
     * Trading is halted.
     */
    static const DXFCPP_EXPORT TradingStatus HALTED;

    /**
     * Trading is active.
     */
    static const DXFCPP_EXPORT TradingStatus ACTIVE;
};

template <>
const std::unordered_map<TradingStatus::CodeType, std::reference_wrapper<const TradingStatus>>
    TradingStatus::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()