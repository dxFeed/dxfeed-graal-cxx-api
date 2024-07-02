// Copyright (c) 2024 Devexperts LLC.
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
 * Side of an order or a trade.
 */
struct Side : Enum<Side, std::uint32_t> {
    using Enum::Enum;

    /**
     * Side is undefined, unknown or inapplicable.
     */
    static const DXFCPP_EXPORT Side UNDEFINED;

    /**
     * Buy side (bid).
     */
    static const DXFCPP_EXPORT Side BUY;

    /**
     * Sell side (ask or offer).
     */
    static const DXFCPP_EXPORT Side SELL;
};

template <> const std::unordered_map<Side::CodeType, std::reference_wrapper<const Side>> Side::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()