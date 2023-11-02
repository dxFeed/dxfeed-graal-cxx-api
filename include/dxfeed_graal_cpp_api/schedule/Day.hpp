// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/**
 * <b>Day</b> represents a continuous period of time approximately 24 hours long. The day is aligned
 * to the start and the end of business activities of a certain business entity or business process.
 * For example, the day may be aligned to a trading schedule of a particular instrument on an exchange.
 * Thus, different days may start and end at various local times depending on the related trading schedules.
 * <p>
 * The length of the day depends on the trading schedule and other circumstances. For example, it is possible
 * that day for Monday is longer than 24 hours because it includes part of Sunday; consequently, the day for
 * Sunday will be shorter than 24 hours to avoid overlapping with Monday.
 * <p>
 * Days do not overlap with each other - rather they form consecutive chain of adjacent periods of time that
 * cover entire time scale. The point on a border line is considered to belong to following day that starts there.
 * <p>
 * Each day consists of sessions that cover entire duration of the day. If day contains at least one trading
 * session (i.e. session within which trading activity is allowed), then the day is considered trading day.
 * Otherwise the day is considered non-trading day (e.g. weekend or holiday).
 * <p>
 * Day may contain sessions with zero duration - e.g. indices that post value once a day.
 * Such sessions can be of any appropriate type, trading or non-trading.
 * Day may have zero duration as well - e.g. when all time within it is transferred to other days.
 */
struct DXFCPP_EXPORT Day {
    friend struct Schedule;
    friend struct Session;

    /// The alias to a type of shared pointer to the Day object
    using Ptr = std::shared_ptr<Day>;

    /// The alias to a type of unique pointer to the Day object
    using Unique = std::unique_ptr<Day>;

  private:
    JavaObjectHandle<Day> handle_;

    explicit Day(void* handle = nullptr) noexcept;

    /**
     * Checks the handle, attempts to allocate memory for the pointer and return Day::Ptr
     *
     * @param handle The graal Day's handle
     * @return The smart pointer for the Day object.
     */
    static Day::Ptr create(void* handle) noexcept;

  public:
    /**
     * @return The schedule to which this day belongs.
     */
    std::shared_ptr<Schedule> getSchedule() const noexcept;

    /**
     * @return The number of this day since January 1, 1970 (that day has identifier of 0 and previous days have
     * negative identifiers).
     */
    std::int32_t getDayId() const noexcept;

    /**
     * Returns year, month and day numbers packed in the following way:
     * <pre>YearMonthDay = year * 10000 + month * 100 + day</pre>
     * For example, September 28, 1977 has value 19770928.
     *
     * @return Packed year, month and day numbers
     */
    std::int32_t getYearMonthDay() const noexcept;

    /**
     * @return The calendar year - i.e. it returns <code>1977</code> for the year <code>1977</code>.
     */
    std::int32_t getYear() const noexcept;

    /**
     * @return The calendar month number in the year starting with <b>1=January</b> and ending with <b>12=December</b>.
     */
    std::int32_t getMonthOfYear() const noexcept;

    /**
     * @return The ordinal day number in the month starting with <b>1</b> for the first day of month.
     */
    std::int32_t getDayOfMonth() const noexcept;

    /**
     * @return The ordinal day number in the week starting with <b>1=Monday</b> and ending with <b>7=Sunday</b>.
     */
    std::int32_t getDayOfWeek() const noexcept;

    /**
     * @return <code>true</code> if this day is an exchange holiday.
     * Usually there are no trading takes place on an exchange holiday.
     */
    bool isHoliday() const noexcept;

    /**
     * @return <code>true</code> if this day is a short day.
     * Usually trading stops earlier on a short day.
     */
    bool isShortDay() const noexcept;

    /**
     * @return <code>true</code> if trading activity is allowed within this day.
     * Positive result assumes that day has at least one trading session.
     */
    bool isTrading() const noexcept;

    /**
     * @return The start time of this day (inclusive).
     */
    std::int64_t getStartTime() const noexcept;

    /**
     * @return The end time of this day (exclusive).
     */
    std::int64_t getEndTime() const noexcept;

    /**
     * @return <code>true</code> if specified time belongs to this day.
     */
    bool containsTime(std::int64_t time) const noexcept;

    /**
     * @return The reset time for this day.
     * Reset of daily data is performed on trading days only, the result has no meaning for non-trading days.
     */
    std::int64_t getResetTime() const noexcept;

    /**
     * Returns following day accepted by specified filter.
     * This method looks for appropriate day up to a year in the future. If no such day was found
     * within one year this method will return <b>Day::Ptr{nullptr}</b>.
     *
     * @param filter the filter to test days
     * @return nearest following day that is accepted by the filter
     */
    Day::Ptr findNextDay(const DayFilter &filter) const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept;
};

} // namespace dxfcpp