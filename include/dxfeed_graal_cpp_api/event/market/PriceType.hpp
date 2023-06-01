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
 * Type of the price value.
 */
struct DXFCPP_EXPORT PriceType : Enum<PriceType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Regular price.
     */
    static const PriceType REGULAR;

    /**
     * Indicative price (derived via math formula).
     */
    static const PriceType INDICATIVE;

    /**
     * Preliminary price (preliminary settlement price), usually posted prior to ::FINAL price.
     */
    static const PriceType PRELIMINARY;

    /**
     * Final price (final settlement price).
     */
    static const PriceType FINAL;
};

} // namespace dxfcpp