// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

DayFilter::DayFilter(DayFilterEnum code, std::string name, std::uint32_t dayOfWeekMask, Tristate holiday,
                     Tristate shortDay, Tristate trading) noexcept
    : code_{code}, name_{std::move(name)}, dayOfWeekMask_{dayOfWeekMask}, holiday_{holiday}, shortDay_{shortDay},
      trading_{trading} {

    handle_ = isolated::schedule::IsolatedDayFilter::getInstance(static_cast<std::uint32_t>(code));
}

/// Accepts any day - useful for pure calendar navigation.
const DayFilter DayFilter::ANY(DayFilterEnum::ANY, "ANY", 0, Tristate::NONE, Tristate::NONE, Tristate::NONE);

/// Accepts trading days only - those with <code>(Day::isTrading() == true)</code>.
const DayFilter DayFilter::TRADING(DayFilterEnum::TRADING, "TRADING", 0, Tristate::NONE, Tristate::NONE,
                                   Tristate::TRUE);

/// Accepts non-trading days only - those with <code>(Day::isTrading() == false)</code>.
const DayFilter DayFilter::NON_TRADING(DayFilterEnum::NON_TRADING, "NON_TRADING", 0, Tristate::NONE, Tristate::NONE,
                                       Tristate::FALSE);

/// Accepts holidays only - those with <code>(Day::isHoliday() == true)</code>.
const DayFilter DayFilter::HOLIDAY(DayFilterEnum::HOLIDAY, "HOLIDAY", 0, Tristate::TRUE, Tristate::NONE,
                                   Tristate::NONE);

/// Accepts short days only - those with <code>(Day::isShortDay() == true)</code>.
const DayFilter DayFilter::SHORT_DAY(DayFilterEnum::SHORT_DAY, "SHORT_DAY", 0, Tristate::NONE, Tristate::TRUE,
                                     Tristate::NONE);

/// Accepts Mondays only - those with <code>(Day::getDayOfWeek() == 1)</code>.
const DayFilter DayFilter::MONDAY(DayFilterEnum::MONDAY, "MONDAY", 1U << 1U, Tristate::NONE, Tristate::NONE,
                                  Tristate::NONE);

/// Accepts Tuesdays only - those with <code>(Day::getDayOfWeek() == 2)</code>.
const DayFilter DayFilter::TUESDAY(DayFilterEnum::TUESDAY, "TUESDAY", 1U << 2U, Tristate::NONE, Tristate::NONE,
                                   Tristate::NONE);

/// Accepts Wednesdays only - those with <code>(Day::getDayOfWeek() == 3)</code>.
const DayFilter DayFilter::WEDNESDAY(DayFilterEnum::WEDNESDAY, "WEDNESDAY", 1U << 3U, Tristate::NONE, Tristate::NONE,
                                     Tristate::NONE);

/// Accepts Thursdays only - those with <code>(Day::getDayOfWeek() == 4)</code>.
const DayFilter DayFilter::THURSDAY(DayFilterEnum::THURSDAY, "THURSDAY", 1U << 4U, Tristate::NONE, Tristate::NONE,
                                    Tristate::NONE);

/// Accepts Fridays only - those with <code>(Day::getDayOfWeek() == 5)</code>.
const DayFilter DayFilter::FRIDAY(DayFilterEnum::FRIDAY, "FRIDAY", 1U << 5U, Tristate::NONE, Tristate::NONE,
                                  Tristate::NONE);

/// Accepts Saturdays only - those with <code>(Day::getDayOfWeek() == 6)</code>.
const DayFilter DayFilter::SATURDAY(DayFilterEnum::SATURDAY, "SATURDAY", 1U << 6U, Tristate::NONE, Tristate::NONE,
                                    Tristate::NONE);

/// Accepts Sundays only - those with <code>(Day::getDayOfWeek() == 7)</code>.
const DayFilter DayFilter::SUNDAY(DayFilterEnum::SUNDAY, "SUNDAY", 1U << 7U, Tristate::NONE, Tristate::NONE,
                                  Tristate::NONE);

/// Accepts week-days only - those with <code>(Day::getDayOfWeek() <= 5)</code>.
const DayFilter DayFilter::WEEK_DAY(DayFilterEnum::WEEK_DAY, "WEEK_DAY", 0x3EU, Tristate::NONE, Tristate::NONE,
                                    Tristate::NONE);

/// Accepts weekends only - those with <code>(Day::getDayOfWeek() >= 6)</code>.
const DayFilter DayFilter::WEEK_END(DayFilterEnum::WEEK_END, "WEEK_END", 0xC0, Tristate::NONE, Tristate::NONE,
                                    Tristate::NONE);

DXFCPP_END_NAMESPACE