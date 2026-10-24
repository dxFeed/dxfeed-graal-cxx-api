// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <ostream>
#include <string>

/**
 * \addtogroup dxfcpp_util
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Immutable value class for a range of time periods: the minimum and the maximum period in milliseconds.
 *
 * It describes the aggregation periods that a server applies (DXEndpoint::getAggregationPeriodInfo()) or that were
 * applied to the delivered events (DXFeedSubscription::getAggregationPeriodInfo()).
 * TimePeriodInfo::UNKNOWN means that no info is available.
 *
 * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/devexperts/util/TimePeriodInfo.html)
 */
struct DXFCPP_EXPORT TimePeriodInfo {
    /// The value meaning "not set / unknown". It has negative minimum and maximum.
    static const TimePeriodInfo UNKNOWN;

    /// The value meaning "known to be zero". It has zero minimum and maximum.
    static const TimePeriodInfo EMPTY;

    /**
     * Returns TimePeriodInfo with the given bounds.
     *
     * @param min The minimum period in milliseconds (non-negative).
     * @param max The maximum period in milliseconds (non-negative, not less than `min`).
     * @return TimePeriodInfo with the given bounds; TimePeriodInfo::EMPTY for (0, 0).
     * @throws InvalidArgumentException if `min` or `max` is negative or `min` is greater than `max`.
     */
    static TimePeriodInfo valueOf(std::int64_t min, std::int64_t max);

    /**
     * Returns the minimum period in milliseconds. It is negative for TimePeriodInfo::UNKNOWN: check isUnknown() before
     * using the value arithmetically.
     *
     * @return The minimum period in milliseconds.
     */
    constexpr std::int64_t getMin() const noexcept {
        return min_;
    }

    /**
     * Returns the maximum period in milliseconds. It is negative for TimePeriodInfo::UNKNOWN: check isUnknown() before
     * using the value arithmetically.
     *
     * @return The maximum period in milliseconds.
     */
    constexpr std::int64_t getMax() const noexcept {
        return max_;
    }

    /**
     * @return `true` if this is TimePeriodInfo::UNKNOWN.
     */
    constexpr bool isUnknown() const noexcept {
        return min_ < 0 && max_ < 0;
    }

    /**
     * Returns the info as JSON with the periods in seconds, as Java does: `{"min":1.5,"max":2}`.
     * TimePeriodInfo::UNKNOWN is `{"min":-1,"max":-1}`.
     *
     * @return The string representation.
     */
    std::string toString() const;

    friend std::ostream &operator<<(std::ostream &os, const TimePeriodInfo &info) {
        return os << info.toString();
    }

    constexpr bool operator==(const TimePeriodInfo &other) const noexcept = default;

    private:
    std::int64_t min_;
    std::int64_t max_;

    constexpr TimePeriodInfo(std::int64_t min, std::int64_t max) noexcept : min_{min}, max_{max} {
    }
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
