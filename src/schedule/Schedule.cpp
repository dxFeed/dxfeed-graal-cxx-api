// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

Schedule::Schedule(JavaObjectHandle<Schedule> &&handle) noexcept : handle_(std::move(handle)) {
}

Schedule::Ptr Schedule::create(JavaObjectHandle<Schedule> &&handle) {
    if (!handle) {
        throw InvalidArgumentException("Unable to create a Schedule object. The handle is nullptr");
    }

    return std::shared_ptr<Schedule>(new Schedule(std::move(handle)));
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile) {
    if (!profile) {
        throw InvalidArgumentException("The profile is nullptr");
    }

    auto schedule =
        create(JavaObjectHandle<Schedule>{isolated::schedule::Schedule::getInstance(profile->handle_.get())});

    return schedule;
}

Schedule::Ptr Schedule::getInstance(const std::string &scheduleDefinition) {
    return create(JavaObjectHandle<Schedule>{isolated::schedule::Schedule::getInstance(scheduleDefinition)});
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile, const std::string &venue) {
    if (!profile) {
        throw InvalidArgumentException("The profile is nullptr");
    }

    auto schedule =
        create(JavaObjectHandle<Schedule>{isolated::schedule::Schedule::getInstance(profile->handle_.get(), venue)});

    return schedule;
}

std::vector<std::string> Schedule::getTradingVenues(std::shared_ptr<InstrumentProfile> profile) {
    if (!profile) {
        throw InvalidArgumentException("The profile is nullptr");
    }

    auto result = isolated::schedule::Schedule::getTradingVenues(profile->handle_.get());

    return result;
}

void Schedule::downloadDefaults(const std::string &downloadConfig) {
    isolated::schedule::Schedule::downloadDefaults(downloadConfig);
}

bool Schedule::setDefaults(const std::vector<char> &data) {
    return isolated::schedule::Schedule::setDefaults(data);
}

std::shared_ptr<Session> Schedule::getSessionByTime(std::int64_t time) const {
    if (!handle_) {
        return {};
    }

    return Session::create(
        JavaObjectHandle<Session>{isolated::schedule::Schedule::getSessionByTime(handle_.get(), time)});
}

std::shared_ptr<Day> Schedule::getDayByTime(std::int64_t time) const {
    if (!handle_) {
        return {};
    }

    return Day::create(JavaObjectHandle<Day>{isolated::schedule::Schedule::getDayByTime(handle_.get(), time)});
}

std::shared_ptr<Day> Schedule::getDayById(std::int32_t dayId) const {
    if (!handle_) {
        return {};
    }

    return Day::create(JavaObjectHandle<Day>{isolated::schedule::Schedule::getDayById(handle_.get(), dayId)});
}

std::shared_ptr<Day> Schedule::getDayByYearMonthDay(std::int32_t yearMonthDay) const {
    if (!handle_) {
        return {};
    }

    return Day::create(
        JavaObjectHandle<Day>{isolated::schedule::Schedule::getDayByYearMonthDay(handle_.get(), yearMonthDay)});
}

std::shared_ptr<Session> Schedule::getNearestSessionByTime(std::int64_t time, const SessionFilter &filter) const {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Schedule::getNearestSessionByTime(handle_.get(), time, filter.getHandle().get())});
}

std::shared_ptr<Session> Schedule::findNearestSessionByTime(std::int64_t time, const SessionFilter &filter) const {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Schedule::findNearestSessionByTime(handle_.get(), time, filter.getHandle().get())});
}

std::string Schedule::getName() const {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getName(handle_.get());
}

std::string Schedule::getTimeZoneDisplayName() const {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getTimeZoneDisplayName(handle_.get());
}

std::string Schedule::getTimeZoneId() const {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getTimeZoneId(handle_.get());
}

DXFCPP_END_NAMESPACE