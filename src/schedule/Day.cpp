// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>

#include <dxfeed_graal_cpp_api/schedule/Day.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp>
#include <dxfeed_graal_cpp_api/schedule/Schedule.hpp>
#include <dxfeed_graal_cpp_api/schedule/Session.hpp>
#include <dxfeed_graal_cpp_api/schedule/SessionFilter.hpp>

#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedDay.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp>

DXFCPP_BEGIN_NAMESPACE

Day::Day(JavaObjectHandle<Day> &&handle) noexcept : handle_(std::move(handle)) {
}

Day::Ptr Day::create(JavaObjectHandle<Day> &&handle) {
    if (!handle) {
        throw InvalidArgumentException("Unable to create a Day object. The handle is invalid");
    }

    return std::shared_ptr<Day>(new Day(std::move(handle)));
}

std::shared_ptr<Schedule> Day::getSchedule() const {
    return Schedule::create(isolated::schedule::IsolatedDay::getSchedule(handle_));
}

std::int32_t Day::getDayId() const {
    return isolated::schedule::IsolatedDay::getDayId(handle_);
}

std::int32_t Day::getYearMonthDay() const {
    return isolated::schedule::IsolatedDay::getYearMonthDay(handle_);
}

std::int32_t Day::getYear() const {
    return isolated::schedule::IsolatedDay::getYear(handle_);
}

std::int32_t Day::getMonthOfYear() const {
    return isolated::schedule::IsolatedDay::getMonthOfYear(handle_);
}

std::int32_t Day::getDayOfMonth() const {
    return isolated::schedule::IsolatedDay::getDayOfMonth(handle_);
}

std::int32_t Day::getDayOfWeek() const {
    return isolated::schedule::IsolatedDay::getDayOfWeek(handle_);
}

bool Day::isHoliday() const {
    return isolated::schedule::IsolatedDay::isHoliday(handle_);
}

bool Day::isShortDay() const {
    return isolated::schedule::IsolatedDay::isShortDay(handle_);
}

bool Day::isTrading() const {
    return isolated::schedule::IsolatedDay::isTrading(handle_);
}

std::int64_t Day::getStartTime() const {
    return isolated::schedule::IsolatedDay::getStartTime(handle_);
}

std::int64_t Day::getEndTime() const {
    return isolated::schedule::IsolatedDay::getEndTime(handle_);
}

bool Day::containsTime(std::int64_t time) const {
    return isolated::schedule::IsolatedDay::containsTime(handle_, time);
}

std::int64_t Day::getResetTime() const {
    return isolated::schedule::IsolatedDay::getResetTime(handle_);
}

std::vector<std::shared_ptr<Session>> Day::getSessions() const {
    auto graalSessionList =
        isolated::schedule::IsolatedSessionList::toUnique(isolated::schedule::IsolatedDay::getSessions(handle_));

    if (!graalSessionList) {
        return {};
    }

    auto *sessionList = dxfcpp::bit_cast<dxfg_session_list *>(graalSessionList.get());
    std::vector<std::shared_ptr<Session>> result;

    if (sessionList->size > 0 && sessionList->elements) {
        result.reserve(static_cast<std::size_t>(sessionList->size));

        for (std::int32_t i = 0; i < sessionList->size; i++) {
            result.emplace_back(Session::create(JavaObjectHandle<Session>(sessionList->elements[i])));
        }
    }

    return result;
}

std::shared_ptr<Session> Day::getSessionByTime(std::int64_t time) const {
    return Session::create(isolated::schedule::IsolatedDay::getSessionByTime(handle_, time));
}

std::shared_ptr<Session> Day::getFirstSession(const SessionFilter &filter) const {
    return Session::create(isolated::schedule::IsolatedDay::getFirstSession(handle_, filter.getHandle()));
}

std::shared_ptr<Session> Day::getLastSession(const SessionFilter &filter) const {
    return Session::create(isolated::schedule::IsolatedDay::getLastSession(handle_, filter.getHandle()));
}

std::shared_ptr<Session> Day::findFirstSession(const SessionFilter &filter) const {
    auto sessionHandle = isolated::schedule::IsolatedDay::findFirstSession(handle_, filter.getHandle());

    if (!sessionHandle) {
        return {};
    }

    return Session::create(std::move(sessionHandle));
}

std::shared_ptr<Session> Day::findLastSession(const SessionFilter &filter) const {
    auto sessionHandle = isolated::schedule::IsolatedDay::findLastSession(handle_, filter.getHandle());

    if (!sessionHandle) {
        return {};
    }

    return Session::create(std::move(sessionHandle));
}

Day::Ptr Day::getPrevDay(const DayFilter &filter) const {
    return create(isolated::schedule::IsolatedDay::getPrevDay(handle_, filter.getHandle()));
}

Day::Ptr Day::getNextDay(const DayFilter &filter) const {
    return create(isolated::schedule::IsolatedDay::getNextDay(handle_, filter.getHandle()));
}

Day::Ptr Day::findPrevDay(const DayFilter &filter) const {
    auto dayHandle = isolated::schedule::IsolatedDay::findPrevDay(handle_, filter.getHandle());

    if (!dayHandle) {
        return {};
    }

    return create(std::move(dayHandle));
}

Day::Ptr Day::findNextDay(const DayFilter &filter) const {
    auto dayHandle = isolated::schedule::IsolatedDay::findNextDay(handle_, filter.getHandle());

    if (!dayHandle) {
        return {};
    }

    return create(std::move(dayHandle));
}

bool Day::operator==(const Day &other) const {
    if (this == &other) {
        return true;
    }

    return isolated::schedule::IsolatedDay::equals(handle_, other.handle_);
}

std::size_t Day::getHashCode() const {
    return isolated::schedule::IsolatedDay::getHashCode(handle_);
}

std::string Day::toString() const {
    return isolated::schedule::IsolatedDay::toString(handle_);
}

DXFCPP_END_NAMESPACE