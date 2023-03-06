// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

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
struct ShortSaleRestriction : Enum<ShortSaleRestriction, std::uint32_t> {
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

    static const std::unordered_map<CodeType, std::reference_wrapper<const ShortSaleRestriction>> ALL;

    /**
     * Returns restriction by integer code bit pattern.
     *
     * @param code integer code.
     * @return restriction.
     */
    template <Integral Code> static const ShortSaleRestriction &valueOf(Code code) {
        if (auto found = ALL.find(static_cast<CodeType>(code)); found != ALL.end()) {
            return found->second;
        }

        return UNDEFINED;
    }
};

} // namespace dxfcpp