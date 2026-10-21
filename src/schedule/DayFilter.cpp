// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/schedule/DayFilter.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/schedule/IsolatedDay.hpp"

#include <dxfg_api.h>

#include <iterator>
DXFCPP_BEGIN_NAMESPACE

namespace {

template <DayFilterEnum code> JavaObjectHandle<DayFilter> createHandle() {
    return isolated::schedule::IsolatedDayFilter::getInstance(static_cast<std::uint32_t>(code));
}

using Handle = LazyJavaObjectHandle<DayFilter>;

// The handles of constants: they are not destroyed at exit.
constexpr Handle::ConstantTag CONSTANT{};

// The Java objects of the filters by code, created at the first use. Not members of the constants: the filters stay
// trivially destructible.
constinit const Handle HANDLES[] = {
    Handle{&createHandle<DayFilterEnum::ANY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::TRADING>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::NON_TRADING>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::HOLIDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::SHORT_DAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::MONDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::TUESDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::WEDNESDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::THURSDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::FRIDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::SATURDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::SUNDAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::WEEK_DAY>, CONSTANT},
    Handle{&createHandle<DayFilterEnum::WEEK_END>, CONSTANT},
};

static_assert(std::size(HANDLES) == static_cast<std::size_t>(DayFilterEnum::WEEK_END) + 1);

} // namespace

const JavaObjectHandle<DayFilter> &DayFilter::getHandle() const & {
    return HANDLES[static_cast<std::size_t>(code_)].get();
}

/// Accepts any day - useful for pure calendar navigation.
constinit const DayFilter DayFilter::ANY(DayFilterEnum::ANY, "ANY", 0, Tristate::NONE, Tristate::NONE, Tristate::NONE);

/// Accepts trading days only - those with <code>(Day::isTrading() == true)</code>.
constinit const DayFilter DayFilter::TRADING(DayFilterEnum::TRADING, "TRADING", 0, Tristate::NONE, Tristate::NONE,
                                             Tristate::TRUE);

/// Accepts non-trading days only - those with <code>(Day::isTrading() == false)</code>.
constinit const DayFilter DayFilter::NON_TRADING(DayFilterEnum::NON_TRADING, "NON_TRADING", 0, Tristate::NONE,
                                                 Tristate::NONE, Tristate::FALSE);

/// Accepts holidays only - those with <code>(Day::isHoliday() == true)</code>.
constinit const DayFilter DayFilter::HOLIDAY(DayFilterEnum::HOLIDAY, "HOLIDAY", 0, Tristate::TRUE, Tristate::NONE,
                                             Tristate::NONE);

/// Accepts short days only - those with <code>(Day::isShortDay() == true)</code>.
constinit const DayFilter DayFilter::SHORT_DAY(DayFilterEnum::SHORT_DAY, "SHORT_DAY", 0, Tristate::NONE, Tristate::TRUE,
                                               Tristate::NONE);

/// Accepts Mondays only - those with <code>(Day::getDayOfWeek() == 1)</code>.
constinit const DayFilter DayFilter::MONDAY(DayFilterEnum::MONDAY, "MONDAY", 1U << 1U, Tristate::NONE, Tristate::NONE,
                                            Tristate::NONE);

/// Accepts Tuesdays only - those with <code>(Day::getDayOfWeek() == 2)</code>.
constinit const DayFilter DayFilter::TUESDAY(DayFilterEnum::TUESDAY, "TUESDAY", 1U << 2U, Tristate::NONE,
                                             Tristate::NONE, Tristate::NONE);

/// Accepts Wednesdays only - those with <code>(Day::getDayOfWeek() == 3)</code>.
constinit const DayFilter DayFilter::WEDNESDAY(DayFilterEnum::WEDNESDAY, "WEDNESDAY", 1U << 3U, Tristate::NONE,
                                               Tristate::NONE, Tristate::NONE);

/// Accepts Thursdays only - those with <code>(Day::getDayOfWeek() == 4)</code>.
constinit const DayFilter DayFilter::THURSDAY(DayFilterEnum::THURSDAY, "THURSDAY", 1U << 4U, Tristate::NONE,
                                              Tristate::NONE, Tristate::NONE);

/// Accepts Fridays only - those with <code>(Day::getDayOfWeek() == 5)</code>.
constinit const DayFilter DayFilter::FRIDAY(DayFilterEnum::FRIDAY, "FRIDAY", 1U << 5U, Tristate::NONE, Tristate::NONE,
                                            Tristate::NONE);

/// Accepts Saturdays only - those with <code>(Day::getDayOfWeek() == 6)</code>.
constinit const DayFilter DayFilter::SATURDAY(DayFilterEnum::SATURDAY, "SATURDAY", 1U << 6U, Tristate::NONE,
                                              Tristate::NONE, Tristate::NONE);

/// Accepts Sundays only - those with <code>(Day::getDayOfWeek() == 7)</code>.
constinit const DayFilter DayFilter::SUNDAY(DayFilterEnum::SUNDAY, "SUNDAY", 1U << 7U, Tristate::NONE, Tristate::NONE,
                                            Tristate::NONE);

/// Accepts week-days only - those with <code>(Day::getDayOfWeek() <= 5)</code>.
constinit const DayFilter DayFilter::WEEK_DAY(DayFilterEnum::WEEK_DAY, "WEEK_DAY", 0x3EU, Tristate::NONE,
                                              Tristate::NONE, Tristate::NONE);

/// Accepts weekends only - those with <code>(Day::getDayOfWeek() >= 6)</code>.
constinit const DayFilter DayFilter::WEEK_END(DayFilterEnum::WEEK_END, "WEEK_END", 0xC0, Tristate::NONE, Tristate::NONE,
                                              Tristate::NONE);

DayFilterEnum DayFilter::getCode() const {
    return code_;
}

std::string DayFilter::getName() const {
    return std::string{name_};
}

std::uint32_t DayFilter::getDayOfWeekMask() const {
    return dayOfWeekMask_;
}

Tristate DayFilter::isHoliday() const {
    return holiday_;
}

Tristate DayFilter::isShortDay() const {
    return shortDay_;
}

Tristate DayFilter::isTrading() const {
    return trading_;
}

DXFCPP_END_NAMESPACE