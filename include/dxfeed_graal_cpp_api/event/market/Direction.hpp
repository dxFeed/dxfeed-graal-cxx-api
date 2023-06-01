// Copyright (c) 2023 Devexperts LLC.
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
 * Direction of the price movement. For example tick direction for last trade price.
 */
struct DXFCPP_EXPORT Direction : Enum<Direction, std::uint32_t> {
    using Enum::Enum;

    /**
     * Direction is undefined, unknown or inapplicable.
     * It includes cases with undefined price value or when direction computation was not performed.
     */
    static const Direction UNDEFINED;

    /**
     * Current price is lower than previous price.
     */
    static const Direction DOWN;

    /**
     * Current price is the same as previous price and is lower than the last known price of different value.
     */
    static const Direction ZERO_DOWN;

    /**
     * Current price is equal to the only known price value suitable for price direction computation.
     * Unlike <b>UNDEFINED</b> the <b>ZERO</b> direction implies that current price is defined and
     * direction computation was duly performed but has failed to detect any upward or downward price movement.
     * It is also reported for cases when price sequence was broken and direction computation was restarted anew.
     */
    static const Direction ZERO;

    /**
     * Current price is the same as previous price and is higher than the last known price of different value.
     */
    static const Direction ZERO_UP;

    /**
     * Current price is higher than previous price.
     */
    static const Direction UP;
};

} // namespace dxfcpp