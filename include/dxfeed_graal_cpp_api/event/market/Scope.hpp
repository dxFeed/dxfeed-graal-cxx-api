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
 * Scope of an order.
 */
struct Scope : Enum<Scope, std::uint32_t> {
    using Enum::Enum;

    /**
     * Represents best bid or best offer for the whole market.
     */
    static const DXFCPP_EXPORT Scope COMPOSITE;

    /**
     * Represents best bid or best offer for a given exchange code.
     */
    static const DXFCPP_EXPORT Scope REGIONAL;

    /**
     * Represents aggregate information for a given price level or
     * best bid or best offer for a given market maker.
     */
    static const DXFCPP_EXPORT Scope AGGREGATE;

    /**
     * Represents individual order on the market.
     */
    static const DXFCPP_EXPORT Scope ORDER;
};

template <> const std::unordered_map<Scope::CodeType, std::reference_wrapper<const Scope>> Scope::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()