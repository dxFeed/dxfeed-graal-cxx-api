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
 * Type of the price value.
 */
struct PriceType : Enum<PriceType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Regular price.
     */
    static const DXFCPP_EXPORT PriceType REGULAR;

    /**
     * Indicative price (derived via math formula).
     */
    static const DXFCPP_EXPORT PriceType INDICATIVE;

    /**
     * Preliminary price (preliminary settlement price), usually posted prior to PriceType::FINAL price.
     */
    static const DXFCPP_EXPORT PriceType PRELIMINARY;

    /**
     * Final price (final settlement price).
     */
    static const DXFCPP_EXPORT PriceType FINAL;
};

template <>
const std::unordered_map<PriceType::CodeType, std::reference_wrapper<const PriceType>> PriceType::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()