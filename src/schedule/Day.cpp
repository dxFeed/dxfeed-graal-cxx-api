// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Day::Day(void *handle) noexcept : handle_(handle) {
}

Day::Ptr Day::create(void *handle) noexcept {
    if (!handle) {
        return {};
    }

    return std::shared_ptr<Day>(new (std::nothrow) Day(handle));
}

std::shared_ptr<Schedule> Day::getSchedule() const noexcept {
    if (!handle_) {
        return {};
    }

    return Schedule::create(isolated::schedule::Day::getSchedule(handle_.get()));
}

std::int32_t Day::getDayId() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getDayId(handle_.get());
}

std::int32_t Day::getYearMonthDay() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getYearMonthDay(handle_.get());
}

std::int32_t Day::getYear() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getYear(handle_.get());
}

std::int32_t Day::getMonthOfYear() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getMonthOfYear(handle_.get());
}

std::int32_t Day::getDayOfMonth() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getDayOfMonth(handle_.get());
}

std::int32_t Day::getDayOfWeek() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getDayOfWeek(handle_.get());
}

bool Day::isHoliday() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Day::isHoliday(handle_.get());
}

bool Day::isShortDay() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Day::isShortDay(handle_.get());
}

bool Day::isTrading() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Day::isTrading(handle_.get());
}

std::int64_t Day::getStartTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getStartTime(handle_.get());
}

std::int64_t Day::getEndTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getEndTime(handle_.get());
}

bool Day::containsTime(std::int64_t time) const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Day::containsTime(handle_.get(), time);
}

std::int64_t Day::getResetTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getResetTime(handle_.get());
}

Day::Ptr Day::findNextDay(const DayFilter &filter) const noexcept {
    if (!handle_ || !filter.handle_) {
        return {};
    }

    return create(isolated::schedule::Day::findNextDay(handle_.get(), filter.handle_.get()));
}

std::string Day::toString() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Day::toString(handle_.get());
}

} // namespace dxfcpp