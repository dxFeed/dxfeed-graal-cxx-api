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
 * Side of an order or a trade.
 */
struct DXFCPP_EXPORT Side : Enum<Side, std::uint32_t> {
    using Enum::Enum;

    /**
     * Side is undefined, unknown or inapplicable.
     */
    static const Side UNDEFINED;

    /**
     * Buy side (bid).
     */
    static const Side BUY;

    /**
     * Sell side (ask or offer).
     */
    static const Side SELL;
};

}