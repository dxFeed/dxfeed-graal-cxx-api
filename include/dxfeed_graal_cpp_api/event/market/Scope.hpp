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
 * Scope of an order.
 */
struct DXFCPP_EXPORT Scope : Enum<Scope, std::uint32_t> {
    using Enum::Enum;

    /**
     * Represents best bid or best offer for the whole market.
     */
    static const Scope COMPOSITE;

    /**
     * Represents best bid or best offer for a given exchange code.
     */
    static const Scope REGIONAL;

    /**
     * Represents aggregate information for a given price level or
     * best bid or best offer for a given market maker.
     */
    static const Scope AGGREGATE;

    /**
     * Represents individual order on the market.
     */
    static const Scope ORDER;
};

template <>
const std::unordered_map<Scope::CodeType, std::reference_wrapper<const Scope>> Scope::ParentType::ALL;

} // namespace dxfcpp