// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

class DXFeedSubscription;

/**
 * Value class for period of time with support for ISO8601 duration format.
 */
struct DXFCPP_EXPORT TimePeriod {
    friend DXFeedSubscription;

    /// Time-period of zero.
    static const TimePeriod ZERO;

    /// Time-period of "infinity" (time of `std::numeric_limits<std::int64_t>::max()` or `LLONG_MAX`).
    static const TimePeriod UNLIMITED;

    /**
     * Returns TimePeriod with <tt>value</tt> milliseconds.
     *
     * @param value value in milliseconds
     * @return TimePeriod with <tt>value</tt> milliseconds.
     */
    static TimePeriod valueOf(std::int64_t value);

    /**
     * Returns TimePeriod with <tt>value</tt> milliseconds.
     *
     * @param value value in milliseconds
     * @return TimePeriod with <tt>value</tt> milliseconds.
     */
    static TimePeriod valueOf(std::chrono::milliseconds value) {
        return valueOf(value.count());
    }

    /**
     * Returns TimePeriod represented with a given string.
     *
     * Allowable format is ISO8601 duration, but there are some simplifications and modifications available:
     * <ul>
     * <li> Letters are case insensitive.
     * <li> Letters "P" and "T" can be omitted.
     * <li> Letter "S" can be also omitted. In this case last number will be supposed to be seconds.
     * <li> Number of seconds can be fractional. So it is possible to define duration accurate within milliseconds.
     * <li> Every part can be omitted. It is supposed that it's value is zero then.
     * <li> String "inf" recognized as unlimited period.
     * </ul>
     *
     * @param value The string representation
     * @return TimePeriod represented with a given string.
     */
    static TimePeriod valueOf(const StringLikeWrapper &value);

    /**
     * Returns value in milliseconds.
     *
     * @return value in milliseconds
     */
    std::int64_t getTime() const;

    /**
     * Returns value in seconds.
     *
     * @return value in seconds
     */
    std::int32_t getSeconds() const;

    /**
     * Returns value in nanoseconds.
     *
     * @return value in nanoseconds
     */
    std::int64_t getNanos() const;

    virtual ~TimePeriod() noexcept = default;

    TimePeriod(const TimePeriod &) = delete;
    TimePeriod(TimePeriod &&) noexcept = delete;
    TimePeriod &operator=(const TimePeriod &) = delete;
    TimePeriod &operator=(const TimePeriod &&) noexcept = delete;

  private:
    JavaObjectHandle<TimePeriod> handle_;

    explicit TimePeriod(JavaObjectHandle<TimePeriod> &&handle);
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()