// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/schedule/Schedule.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"
#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfile.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/schedule/IsolatedSchedule.hpp"
#include "../../include/dxfeed_graal_cpp_api/schedule/Day.hpp"
#include "../../include/dxfeed_graal_cpp_api/schedule/Session.hpp"
#include "../../include/dxfeed_graal_cpp_api/schedule/SessionFilter.hpp"

DXFCPP_BEGIN_NAMESPACE

Schedule::Schedule(JavaObjectHandle<Schedule> &&handle) noexcept : handle_(std::move(handle)) {
}

Schedule::Ptr Schedule::create(JavaObjectHandle<Schedule> &&handle) {
    if (!handle) {
        throw InvalidArgumentException("Unable to create a Schedule object. The handle is nullptr");
    }

    // ReSharper disable once CppDFAMemoryLeak
    return std::shared_ptr<Schedule>(new Schedule(std::move(handle)));
}

Schedule::Ptr Schedule::getInstance(const std::shared_ptr<InstrumentProfile> &profile) {
    if (!profile) {
        throw InvalidArgumentException("The `profile` is nullptr");
    }

    return create(isolated::schedule::IsolatedSchedule::getInstance(profile->handle_));
}

Schedule::Ptr Schedule::getInstance(const StringLike &scheduleDefinition) {
    return create(isolated::schedule::IsolatedSchedule::getInstance(scheduleDefinition));
}

Schedule::Ptr Schedule::getInstance(const std::shared_ptr<InstrumentProfile> &profile, const StringLike &venue) {
    if (!profile) {
        throw InvalidArgumentException("The `profile` is nullptr");
    }

    return create(isolated::schedule::IsolatedSchedule::getInstance(profile->handle_, venue));
}

std::vector<std::string> Schedule::getTradingVenues(const std::shared_ptr<InstrumentProfile> &profile) {
    if (!profile) {
        throw InvalidArgumentException("The profile is nullptr");
    }

    return isolated::schedule::IsolatedSchedule::getTradingVenues(profile->handle_);
}

void Schedule::downloadDefaults(const StringLike &downloadConfig) {
    isolated::schedule::IsolatedSchedule::downloadDefaults(downloadConfig);
}

void Schedule::setDefaults(const std::vector<char> &data) {
    return isolated::schedule::IsolatedSchedule::setDefaults(data);
}

std::shared_ptr<Session> Schedule::getSessionByTime(std::int64_t time) const {
    return Session::create(isolated::schedule::IsolatedSchedule::getSessionByTime(handle_, time));
}

std::shared_ptr<Day> Schedule::getDayByTime(std::int64_t time) const {
    return Day::create(isolated::schedule::IsolatedSchedule::getDayByTime(handle_, time));
}

std::shared_ptr<Day> Schedule::getDayById(std::int32_t dayId) const {
    return Day::create(isolated::schedule::IsolatedSchedule::getDayById(handle_, dayId));
}

std::shared_ptr<Day> Schedule::getDayByYearMonthDay(std::int32_t yearMonthDay) const {
    return Day::create(isolated::schedule::IsolatedSchedule::getDayByYearMonthDay(handle_, yearMonthDay));
}

std::shared_ptr<Session> Schedule::getNearestSessionByTime(std::int64_t time, const SessionFilter &filter) const {
    return Session::create(
        isolated::schedule::IsolatedSchedule::getNearestSessionByTime(handle_, time, filter.getHandle()));
}

std::shared_ptr<Session> Schedule::findNearestSessionByTime(std::int64_t time, const SessionFilter &filter) const {
    auto sessionHandle =
        isolated::schedule::IsolatedSchedule::findNearestSessionByTime(handle_, time, filter.getHandle());

    if (!sessionHandle) {
        return {};
    }

    return Session::create(std::move(sessionHandle));
}

std::string Schedule::getName() const {
    return isolated::schedule::IsolatedSchedule::getName(handle_);
}

std::string Schedule::getTimeZoneDisplayName() const {
    return isolated::schedule::IsolatedSchedule::getTimeZoneDisplayName(handle_);
}

std::string Schedule::getTimeZoneId() const {
    return isolated::schedule::IsolatedSchedule::getTimeZoneId(handle_);
}

DXFCPP_END_NAMESPACE