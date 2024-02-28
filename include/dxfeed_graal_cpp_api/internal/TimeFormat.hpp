// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "JavaObjectHandle.hpp"

#include <cstddef>
#include <cstdint>
#include <locale>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <concepts>

namespace dxfcpp {

/**
 * Utility class for parsing and formatting dates and times in ISO-compatible format.
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
    JavaObjectHandle<TimeFormat> handle_;

  public:
    explicit TimeFormat(void* handle = nullptr) noexcept;
    explicit TimeFormat(JavaObjectHandle<TimeFormat>&& handle) noexcept;
    virtual ~TimeFormat() noexcept = default;

    TimeFormat(const TimeFormat &) = delete;
    TimeFormat(TimeFormat &&) noexcept = delete;
    TimeFormat &operator=(const TimeFormat &) = delete;
    TimeFormat &operator=(const TimeFormat &&) noexcept = delete;

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
    std::int64_t parse(const std::string& value) const noexcept;

    /**
     * Converts timestamp into string according to the format
     *
     * @param timestamp The date and time timestamp.
     * @return string representation of data and time or empty string on error
     */
    std::string format(std::int64_t timestamp) const noexcept;
};

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()