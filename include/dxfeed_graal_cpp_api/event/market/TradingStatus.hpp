// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

namespace dxfcpp {

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

} // namespace dxfcpp