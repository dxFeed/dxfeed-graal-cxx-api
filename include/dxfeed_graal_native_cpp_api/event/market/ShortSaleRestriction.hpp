// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <string>

#include "../../internal/Common.hpp"

namespace dxfcpp {

/**
 * Short sale restriction on an instrument.
 */
struct ShortSaleRestriction {
    using CodeType = std::uint32_t;

  private:
    CodeType code_;
    std::string name_;

    template <Integral Code>
    explicit ShortSaleRestriction(Code code, std::string name) noexcept
        : code_{static_cast<CodeType>(code)}, name_{std::move(name)} {}

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

    /**
     * Returns code that is used in flag bits.
     *
     * @return code.
     */
    [[nodiscard]] CodeType getCode() const { return code_; }

    /**
     * Returns the enum element name
     *
     * @return name
     */
    [[nodiscard]] const std::string &getName() const & { return name_; }

    /**
     * Compares two elements of an enum.
     *
     * @param other An other enum element
     * @return `true` if the elements are the same
     */
    bool operator==(const ShortSaleRestriction &other) const { return code_ == other.code_; }

    /**
     * Returns a string representation of an enum element.
     *
     * @return a string representation
     */
    [[nodiscard]] const std::string &toString() const &noexcept { return name_; }

    /**
     * Puts the string representation of an enum element to the output stream.
     *
     * @tparam OStream A type of the output stream
     * @param os The output stream
     * @param ssr An enum element
     * @return The output stream
     */
    template <typename OStream> friend OStream &operator<<(OStream &&os, const ShortSaleRestriction &ssr) {
        return os << ssr.toString();
    }
};

} // namespace dxfcpp