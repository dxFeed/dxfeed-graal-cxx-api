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
 * Short sale restriction on an instrument.
 */
struct DXFCPP_EXPORT ShortSaleRestriction : Enum<ShortSaleRestriction, std::uint32_t> {
    using Enum::Enum;

    /**
     * Short sale restriction is undefined, unknown or inapplicable.
     */
    static const ShortSaleRestriction UNDEFINED;

    /**
     * Short sale restriction is active.
     */
    static const ShortSaleRestriction ACTIVE;

    /**
     * Short sale restriction is inactive.
     */
    static const ShortSaleRestriction INACTIVE;
};

} // namespace dxfcpp