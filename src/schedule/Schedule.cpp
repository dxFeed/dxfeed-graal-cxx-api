// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

Schedule::Schedule(void *handle) noexcept : handle_(handle) {
}

Schedule::Ptr Schedule::create(void *handle) {
    if (!handle) {
        throw std::invalid_argument(
            "Unable to create a Schedule object. The handle is nullptr");
    }

    return std::shared_ptr<Schedule>(new Schedule(handle));
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile) noexcept {
    if (!profile) {
        // TODO: error handling [EN-8232]
        return {};
    }

    auto graalProfile = profile->toGraal();
    auto schedule = create(isolated::schedule::Schedule::getInstance(graalProfile));
    InstrumentProfile::freeGraal(graalProfile);

    return schedule;
}

Schedule::Ptr Schedule::getInstance(const std::string &scheduleDefinition) noexcept {
    return create(isolated::schedule::Schedule::getInstance(scheduleDefinition));
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile, const std::string &venue) noexcept {
    if (!profile) {
        // TODO: error handling [EN-8232]
        return {};
    }

    auto graalProfile = profile->toGraal();
    auto schedule = create(isolated::schedule::Schedule::getInstance(graalProfile, venue));
    InstrumentProfile::freeGraal(graalProfile);

    return schedule;
}

std::vector<std::string> Schedule::getTradingVenues(std::shared_ptr<InstrumentProfile> profile) noexcept {
    if (!profile) {
        // TODO: error handling [EN-8232]
        return {};
    }

    auto graalProfile = profile->toGraal();
    auto result = isolated::schedule::Schedule::getTradingVenues(graalProfile);
    InstrumentProfile::freeGraal(graalProfile);

    return result;
}

void Schedule::downloadDefaults(const std::string &downloadConfig) noexcept {
    isolated::schedule::Schedule::downloadDefaults(downloadConfig);
}

bool Schedule::setDefaults(const std::vector<char> &data) noexcept {
    return isolated::schedule::Schedule::setDefaults(data);
}

Session::Ptr Schedule::getSessionByTime(std::int64_t time) const noexcept {
    if (!handle_) {
        return {};
    }

    return Session::create(isolated::schedule::Schedule::getSessionByTime(handle_.get(), time));
}

Day::Ptr Schedule::getDayByTime(std::int64_t time) const noexcept {
    if (!handle_) {
        return {};
    }

    return Day::create(isolated::schedule::Schedule::getDayByTime(handle_.get(), time));
}

Day::Ptr Schedule::getDayById(std::int32_t dayId) const noexcept {
    if (!handle_) {
        return {};
    }

    return Day::create(isolated::schedule::Schedule::getDayById(handle_.get(), dayId));
}

Day::Ptr Schedule::getDayByYearMonthDay(std::int32_t yearMonthDay) const noexcept {
    if (!handle_) {
        return {};
    }

    return Day::create(isolated::schedule::Schedule::getDayByYearMonthDay(handle_.get(), yearMonthDay));
}

Session::Ptr Schedule::getNearestSessionByTime(std::int64_t time, const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.handle_) {
        return {};
    }

    return Session::create(
        isolated::schedule::Schedule::getNearestSessionByTime(handle_.get(), time, filter.handle_.get()));
}

Session::Ptr Schedule::findNearestSessionByTime(std::int64_t time, const SessionFilter& filter) const noexcept {
    if (!handle_ || !filter.handle_) {
        return {};
    }

    return Session::create(
        isolated::schedule::Schedule::findNearestSessionByTime(handle_.get(), time, filter.handle_.get()));
}

std::string Schedule::getName() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getName(handle_.get());
}

std::string Schedule::getTimeZoneDisplayName() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getTimeZoneDisplayName(handle_.get());
}

std::string Schedule::getTimeZoneId() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Schedule::getTimeZoneId(handle_.get());
}

DXFCPP_END_NAMESPACE