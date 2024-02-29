// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

DXFCPP_BEGIN_NAMESPACE

/// Helper enum for DayFilter
enum class DayFilterEnum : std::uint32_t {
    /// Accepts any day - useful for pure calendar navigation.
    ANY = 0,

    /// Accepts trading days only - those with <code>(Day::isTrading() == true)</code>.
    TRADING,

    /// Accepts non-trading days only - those with <code>(Day::isTrading() == false)</code>.
    NON_TRADING,

    /// Accepts holidays only - those with <code>(Day::isHoliday() == true)</code>.
    HOLIDAY,

    /// Accepts short days only - those with <code>(Day::isShortDay() == true)</code>.
    SHORT_DAY,

    /// Accepts Mondays only - those with <code>(Day::getDayOfWeek() == 1)</code>.
    MONDAY,

    /// Accepts Tuesdays only - those with <code>(Day::getDayOfWeek() == 2)</code>.
    TUESDAY,

    /// Accepts Wednesdays only - those with <code>(Day::getDayOfWeek() == 3)</code>.
    WEDNESDAY,

    /// Accepts Thursdays only - those with <code>(Day::getDayOfWeek() == 4)</code>.
    THURSDAY,

    /// Accepts Fridays only - those with <code>(Day::getDayOfWeek() == 5)</code>.
    FRIDAY,

    /// Accepts Saturdays only - those with <code>(Day::getDayOfWeek() == 6)</code>.
    SATURDAY,

    /// Accepts Sundays only - those with <code>(Day::getDayOfWeek() == 7)</code>.
    SUNDAY,

    /// Accepts week-days only - those with <code>(Day::getDayOfWeek() <= 5)</code>.
    WEEK_DAY,

    /// Accepts weekends only - those with <code>(Day::getDayOfWeek() >= 6)</code>.
    WEEK_END,
};

/**
 * A filter for days used by various search methods.
 * This class provides predefined filters for certain Day attributes,
 * although users can create their own filters to suit their needs.
 * <p>
 * Please note that days can be either trading or non-trading, and this distinction can be
 * either based on rules (e.g. weekends) or dictated by special occasions (e.g. holidays).
 * Different filters treat this distinction differently - some accept only trading days,
 * some only non-trading, and some ignore type of day altogether.
 */
struct DXFCPP_EXPORT DayFilter {
    friend struct Day;

    /// Accepts any day - useful for pure calendar navigation.
    static const DayFilter ANY;

    /// Accepts trading days only - those with <code>(Day::isTrading() == true)</code>.
    static const DayFilter TRADING;

    /// Accepts non-trading days only - those with <code>(Day::isTrading() == false)</code>.
    static const DayFilter NON_TRADING;

    /// Accepts holidays only - those with <code>(Day::isHoliday() == true)</code>.
    static const DayFilter HOLIDAY;

    /// Accepts short days only - those with <code>(Day::isShortDay() == true)</code>.
    static const DayFilter SHORT_DAY;

    /// Accepts Mondays only - those with <code>(Day::getDayOfWeek() == 1)</code>.
    static const DayFilter MONDAY;

    /// Accepts Tuesdays only - those with <code>(Day::getDayOfWeek() == 2)</code>.
    static const DayFilter TUESDAY;

    /// Accepts Wednesdays only - those with <code>(Day::getDayOfWeek() == 3)</code>.
    static const DayFilter WEDNESDAY;

    /// Accepts Thursdays only - those with <code>(Day::getDayOfWeek() == 4)</code>.
    static const DayFilter THURSDAY;

    /// Accepts Fridays only - those with <code>(Day::getDayOfWeek() == 5)</code>.
    static const DayFilter FRIDAY;

    /// Accepts Saturdays only - those with <code>(Day::getDayOfWeek() == 6)</code>.
    static const DayFilter SATURDAY;

    /// Accepts Sundays only - those with <code>(Day::getDayOfWeek() == 7)</code>.
    static const DayFilter SUNDAY;

    /// Accepts week-days only - those with <code>(Day::getDayOfWeek() <= 5)</code>.
    static const DayFilter WEEK_DAY;

    /// Accepts weekends only - those with <code>(Day::getDayOfWeek() >= 6)</code>.
    static const DayFilter WEEK_END;

    DayFilter(const DayFilter &) = delete;
    DayFilter(DayFilter &&) noexcept = delete;
    DayFilter &operator=(const DayFilter &) = delete;
    DayFilter &operator=(const DayFilter &&) noexcept = delete;

  private:
    DayFilterEnum code_;

    std::string name_;

    /// Bitwise mask of required days (2 for Monday, 4 for Tuesday, 128 for Sunday), <code>0</code> if not relevant.
    std::uint32_t dayOfWeekMask_;
    /// Required holiday flag, Tristate::NONE if not relevant.
    Tristate holiday_;
    /// Required short day flag, Tristate::NONE if not relevant.
    Tristate shortDay_;
    /// Required trading flag, Tristate::NONE if not relevant.
    Tristate trading_;

    JavaObjectHandle<DayFilter> handle_;

    /**
     * Creates filter with specified conditions.
     * <p>
     * The <code>dayOfWeekMask</code> is a bitwise mask with individual bits for each day of week.
     * For the day of week number N the N'th bit is used - the Day will be accepted if corresponding bit is set.
     * If no bits is set (if mask is zero) then day of week attribute is ignored (any value is accepted).
     * <p>
     * The boolean parameters specify what value corresponding attributes should have.
     * If some parameter is Tristate::NONE then corresponding attribute is ignored (any value is accepted).
     *
     * @param code The enum code
     * @param name The enum name
     * @param dayOfWeekMask The bitwise mask of required days (2 for Monday, 4 for Tuesday, 128 for Sunday),
     * <code>0</code> if not relevant
     * @param holiday The required holiday flag, Tristate::NONE if not relevant
     * @param shortDay The required short day flag, Tristate::NONE if not relevant
     * @param trading The required trading flag, Tristate::NONE if not relevant
     */
    DayFilter(DayFilterEnum code, std::string name, std::uint32_t dayOfWeekMask, Tristate holiday, Tristate shortDay,
              Tristate trading) noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()