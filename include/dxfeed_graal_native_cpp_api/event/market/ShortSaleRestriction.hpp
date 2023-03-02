// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>

namespace dxfcpp {

/**
 * Short sale restriction on an instrument.
 */
class ShortSaleRestriction {

    std::int32_t code_;

    explicit ShortSaleRestriction(std::int32_t code) noexcept : code_{code} {}

  public:
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

    static const std::unordered_map<std::int32_t, ShortSaleRestriction> ALL;

    static ShortSaleRestriction valueOf(std::int32_t code) {
        if (auto found = ALL.find(code); found != ALL.end()) {
            return found->second;
        }

        return UNDEFINED;
    }

    /**
     * Returns integer code that is used in flag bits.
     *
     * @return integer code.
     */
    std::int32_t getCode() const { return code_; }

    bool operator == (const ShortSaleRestriction& other) const {
        return code_ == other.code_;
    }
};

} // namespace dxfcpp