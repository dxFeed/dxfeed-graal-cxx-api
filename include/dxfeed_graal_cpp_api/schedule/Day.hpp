// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

struct Schedule;
struct Session;
struct SessionFilter;

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

    explicit Day(void *handle = nullptr) noexcept;

    /**
     * Checks the handle, attempts to allocate memory for the pointer and return Day::Ptr
     *
     * @param handle The graal Day's handle
     * @return The smart pointer for the Day object.
     */
    static Day::Ptr create(void *handle);

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
     * @return collection of sessions that constitute this day.
     * The list is ordered according to natural order of sessions - how they occur one after another.
     */
    std::vector<std::shared_ptr<Session>> getSessions() const;

    /**
     * Returns session belonging to this day that contains specified time.
     * If no such session was found within this day this method will return Session::Ptr{nullptr}
     * (std::shared_ptr<Session>{nullptr}).
     *
     * @param time The time to search for
     * @return The session that contains specified time or Session::Ptr{nullptr} (std::shared_ptr<Session>{nullptr})
     * if no such session was found within this day
     */
    std::shared_ptr<Session> getSessionByTime(std::int64_t time) const noexcept;

    /**
     * Returns first session belonging to this day accepted by specified filter.
     * This method does not cross the day boundary. If no such session was found
     * within this day this method will return Session::Ptr{nullptr} (std::shared_ptr<Session>{nullptr})
     * <p>
     * To find first trading session of any type use this code:
     * <pre>Session session = day->getFirstSession(SessionFilter::TRADING);</pre>
     * To find first regular trading session use this code:
     * <pre>Session session = day->getFirstSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The first session that is accepted by the filter or Session::Ptr{nullptr}
     * (std::shared_ptr<Session>{nullptr}) if no such session was found within this day
     */
    std::shared_ptr<Session> getFirstSession(const SessionFilter &filter) const noexcept;

    /**
     * Returns last session belonging to this day accepted by specified filter.
     * This method does not cross the day boundary. If no such session was found
     * within this day this method will return Session::Ptr{nullptr} (std::shared_ptr<Session>{nullptr})
     * <p>
     * To find last trading session of any type use this code:
     * <pre>auto session = day->getLastSession(SessionFilter::TRADING);</pre>
     * To find last regular trading session use this code:
     * <pre>auto session = day->getLastSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The last session that is accepted by the filter or Session::Ptr{nullptr}
     * (std::shared_ptr<Session>{nullptr}) if no such session was found within this day
     */
    std::shared_ptr<Session> getLastSession(const SessionFilter &filter) const noexcept;

    /**
     * Returns first session belonging to this day accepted by specified filter.
     * This method does not cross the day boundary. If no such session was found
     * within this day this method will return Session::Ptr{nullptr} (std::shared_ptr<Session>{nullptr})
     * <p>
     * To find first trading session of any type use this code:
     * <pre>auto session = day->findFirstSession(SessionFilter::TRADING);</pre>
     * To find first regular trading session use this code:
     * <pre>auto session = day->findFirstSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The first session that is accepted by the filter
     */
    std::shared_ptr<Session> findFirstSession(const SessionFilter &filter) const noexcept;

    /**
     * Returns last session belonging to this day accepted by specified filter.
     * This method does not cross the day boundary. If no such session was found
     * within this day this method will return Session::Ptr{nullptr} (std::shared_ptr<Session>{nullptr})
     * <p>
     * To find last trading session of any type use this code:
     * <pre>auto session = day->findLastSession(SessionFilter::TRADING);</pre>
     * To find last regular trading session use this code:
     * <pre>auto session = day->findLastSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The last session that is accepted by the filter
     */
    std::shared_ptr<Session> findLastSession(const SessionFilter &filter) const noexcept;

    /**
     * Returns previous day accepted by specified filter.
     * This method looks for appropriate day up to a year back in time. If no such day was found
     * within one year this method will return Day::Ptr{nullptr} (std::shared_ptr<Day>{nullptr}).
     *
     * @param The filter the filter to test days
     * @return The nearest previous day that is accepted by the filter or Day::Ptr{nullptr}
     * (std::shared_ptr<Day>{nullptr}) if no such day was found within one year
     */
    Day::Ptr getPrevDay(const DayFilter &filter) const noexcept;

    /**
     * Returns following day accepted by specified filter.
     * This method looks for appropriate day up to a year in the future. If no such day was found
     * within one year this method will return Day::Ptr{nullptr} (std::shared_ptr<Day>{nullptr}).
     *
     * @param The filter the filter to test days
     * @return The nearest following day that is accepted by the filter or Day::Ptr{nullptr}
     * (std::shared_ptr<Day>{nullptr}) if no such day was found within one year
     */
    Day::Ptr getNextDay(const DayFilter &filter) const noexcept;

    /**
     * Returns previous day accepted by specified filter.
     * This method looks for appropriate day up to a year back in time. If no such day was found
     * within one year this method will return Day::Ptr{nullptr} (std::shared_ptr<Day>{nullptr}).
     *
     * @param filter The filter to test days
     * @return The nearest previous day that is accepted by the filter
     */
    Day::Ptr findPrevDay(const DayFilter &filter) const noexcept;

    /**
     * Returns following day accepted by specified filter.
     * This method looks for appropriate day up to a year in the future. If no such day was found
     * within one year this method will return Day::Ptr{nullptr} (std::shared_ptr<Day>{nullptr}).
     *
     * @param filter The filter to test days
     * @return The nearest following day that is accepted by the filter
     */
    Day::Ptr findNextDay(const DayFilter &filter) const noexcept;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const Day &other) const noexcept;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const Day::Ptr &other) const noexcept {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t getHashCode() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation
     */
    std::string toString() const;
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::Day> {
    std::size_t operator()(const dxfcpp::Day &day) const noexcept {
        return day.getHashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()