// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * Direction of the price movement. For example, tick a direction for the last trade price.
 */
struct Direction : Enum<Direction, std::uint32_t> {
    using Enum::Enum;

    /**
     * Direction is undefined, unknown or inapplicable.
     * It includes cases with undefined price value or when direction computation was not performed.
     */
    static const DXFCPP_EXPORT Direction UNDEFINED;

    /**
     * The current price is lower than the previous price.
     */
    static const DXFCPP_EXPORT Direction DOWN;

    /**
     * The current price is the same as the previous price and is lower than the last known price of different value.
     */
    static const DXFCPP_EXPORT Direction ZERO_DOWN;

    /**
     * The current price is equal to the only known price value suitable for price direction computation.
     * Unlike <b>UNDEFINED</b> the <b>ZERO</b> direction implies that the current price is defined and
     * direction computation was duly performed but has failed to detect any upward or downward price movement.
     * It is also reported for cases when a price sequence was broken and direction computation was restarted anew.
     */
    static const DXFCPP_EXPORT Direction ZERO;

    /**
     * The current price is the same as the previous price and is higher than the last known price of different value.
     */
    static const DXFCPP_EXPORT Direction ZERO_UP;

    /**
     * The current price is higher than the previous price.
     */
    static const DXFCPP_EXPORT Direction UP;
};

template <>
const std::unordered_map<Direction::CodeType, std::reference_wrapper<const Direction>> Direction::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()