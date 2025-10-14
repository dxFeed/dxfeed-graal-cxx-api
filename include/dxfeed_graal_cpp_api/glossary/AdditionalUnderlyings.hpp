// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Common.hpp"
#include "../internal/Conf.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <map>
#include <string>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Represents a set of additional underlyings for a given option. Each additional underlying
 * has an associated parameter called SPC (shares per contract) that specifies how many shares
 * of additional underlying are delivered during settlement of the option. In cases when
 * the option delivers additional cash, the SPC specifies an amount of how much cash is delivered.
 *
 * See AdditionalUnderlyings::getText() and AdditionalUnderlyings::getMap() for details about used formats and
 * representations.
 */
struct DXFCPP_EXPORT AdditionalUnderlyings : RequireMakeShared<AdditionalUnderlyings> {
    /// The alias to a type of shared pointer to the AdditionalUnderlyings object
    using Ptr = std::shared_ptr<AdditionalUnderlyings>;

    /// The alias to a type of unique pointer to the AdditionalUnderlyings object
    using Unique = std::unique_ptr<AdditionalUnderlyings>;

    private:
    JavaObjectHandle<AdditionalUnderlyings> handle_;

    explicit AdditionalUnderlyings(LockExternalConstructionTag,
                                   JavaObjectHandle<AdditionalUnderlyings> &&handle) noexcept;

    public:
    ~AdditionalUnderlyings() noexcept override;

    static Ptr valueOf(const StringLikeWrapper &value);

    static Ptr valueOf(const std::unordered_map<std::string, double> &map);

    static Ptr valueOf(const std::map<std::string, double> &map);

    static double getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol);

    std::string getText() const;

    std::unordered_map<std::string, double> getMap() const;

    double getSPC(const StringLikeWrapper &symbol) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const AdditionalUnderlyings &other) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const AdditionalUnderlyings::Ptr &other) const {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t hashCode() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation
     */
    std::string toString() const;

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings &au) {
        return os << au.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings::Ptr &au) {
        return os << au->toString();
    }
};

DXFCPP_END_NAMESPACE