// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "JavaObjectHandle.hpp"

#include <cstdint>
#include <string>
#include <functional>

DXFCPP_BEGIN_NAMESPACE

struct StringLikeWrapper;

/**
 * Utility class for parsing and formatting dates and times in ISO-compatible format.
 *
 * Some methods that are not marked `noexcept` may throw exceptions:
 *
 * @throws InvalidArgumentException if handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend
 * @throws GraalException if something happened with the GraalVM
 */
struct DXFCPP_EXPORT TimeFormat {
    /**
     * An instance of TimeFormat that corresponds to default timezone as returned by TimeZone::getDefault() method.
     */
    const static TimeFormat DEFAULT;
    const static TimeFormat DEFAULT_WITH_MILLIS;
    const static TimeFormat DEFAULT_WITH_MILLIS_WITH_TIMEZONE;

    /**
     * An instance of TimeFormat that corresponds to GMT timezone as returned by
     * @ref TimeZone::getTimeZone() "TimeZone::getTimeZone(\"GMT\")".
     */
    const static TimeFormat GMT;

  private:
    mutable JavaObjectHandle<TimeFormat> handle_;
    mutable std::mutex mtx_{};
    mutable bool initialized_{};
    std::function<JavaObjectHandle<TimeFormat>()> initializer_;

    //lazy c-tor
    explicit TimeFormat(std::function<JavaObjectHandle<TimeFormat>()> &&initializer);

    void init() const;

  public:
    virtual ~TimeFormat() noexcept = default;

    TimeFormat(const TimeFormat &) = delete;
    TimeFormat(TimeFormat &&) noexcept = delete;
    TimeFormat &operator=(const TimeFormat &) = delete;
    TimeFormat &operator=(const TimeFormat &&) noexcept = delete;

    const JavaObjectHandle<TimeFormat> &getHandle() const;

    /**
     * Reads Date from String and returns timestamp.
     * This method is designed to understand
     * <a href="http://en.wikipedia.org/wiki/ISO_8601">ISO 8601</a> formatted date and time.
     * It accepts the following formats:
     *
     * <ul>
     * <li>
     * <b><tt>0</tt></b> is parsed as zero time.
     * </li>
     * <li>
     * <b><tt>'-'&lt;time-period&gt;</tt></b> it is parsed as &lt;current time&gt; - &lt;time-period&gt;.
     * </li>
     * <li>
     * <b><tt>&lt;long-value-in-milliseconds&gt;</tt></b> It should be positive and have at least 9 digits
     * (otherwise it could not be distinguished from date in format <tt>'yyyymmdd'</tt>).
     * Each date since 1970-01-03 can be represented in this form.
     * </li>
     * <li>
     * <b><tt>&lt;date&gt;[('T'|'t'|'-'|' ')&lt;time&gt;][&lt;timezone&gt;]</tt></b>
     * If time is missing it is supposed to be <tt>'00:00:00'</tt>.
     * </li>
     * <li>
     * <b><tt>['T'|'t']&lt;time&gt;[&lt;timezone&gt;]</tt></b>
     * In this case current date is used.
     * </li>
     * </ul>
     * Here
     * <ul>
     * <li>
     * <b><tt>&lt;date&gt;</tt></b> is one of
     *     <ul>
     *     <li><b><tt>yyyy-MM-dd</tt></b>
     *     <li><b><tt>yyyyMMdd</tt></b>
     *     </ul>
     * </li>
     * <li>
     * <b><tt>&lt;time&gt;</tt></b> is one of
     *     <ul>
     *     <li><b><tt>HH:mm:ss[.sss]</tt></b>
     *     <li><b><tt>HHmmss[.sss]</tt></b>
     *     <li><b><tt>HH:mm</tt></b>
     *     <li><b><tt>HHmm</tt></b>
     *     <li><b><tt>HH</tt></b>
     *     </ul>
     * </li>
     * <li>
     * <b><tt>&lt;timezone&gt;</tt></b> is one of
     *     <ul>
     *     <li><b><tt>[+-]HH:mm</tt></b>
     *     <li><b><tt>[+-]HHmm</tt></b>
     *     <li><b><tt>[+-]HH</tt></b>
     *     <li><b><tt>Z</tt></b> for UTC.
     *     <li>or any timezone that can be parsed by Java SimpleDateFormat.
     *     </ul>
     * </li>
     * </ul>
     *
     * @param value String value to parse.
     * @return Date's timestamp parsed from <tt>value</tt> or `0` if <tt>value</tt> has wrong format.
     */
    std::int64_t parse(const StringLikeWrapper &value) const;

    /**
     * Converts timestamp into string according to the format
     *
     * @param timestamp The date and time timestamp.
     * @return string representation of data and time or empty string on error
     */
    std::string format(std::int64_t timestamp) const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()