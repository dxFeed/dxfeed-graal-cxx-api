// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Schedule::Schedule() noexcept : handle_{nullptr} {
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile) noexcept {
    Schedule::Ptr schedule{new (std::nothrow) Schedule{}};

    if (!schedule || !profile) {
        // TODO: error handling
        return schedule;
    }

    auto graalProfile = profile->toGraal();
    schedule->handle_ = isolated::schedule::Schedule::getInstance(graalProfile);

    if (!schedule->handle_) {
        // TODO: error handling
    }

    InstrumentProfile::freeGraal(graalProfile);

    return schedule;
}

Schedule::Ptr Schedule::getInstance(const std::string &scheduleDefinition) noexcept {
    Schedule::Ptr schedule{new (std::nothrow) Schedule{}};

    if (!schedule) {
        // TODO: error handling
        return schedule;
    }

    schedule->handle_ = isolated::schedule::Schedule::getInstance(scheduleDefinition);

    if (!schedule->handle_) {
        // TODO: error handling
    }

    return schedule;
}

Schedule::Ptr Schedule::getInstance(std::shared_ptr<InstrumentProfile> profile, const std::string &venue) noexcept {
    Schedule::Ptr schedule{new (std::nothrow) Schedule{}};

    if (!schedule || !profile) {
        // TODO: error handling
        return schedule;
    }

    auto graalProfile = profile->toGraal();
    schedule->handle_ = isolated::schedule::Schedule::getInstance(graalProfile, venue);

    if (!schedule->handle_) {
        // TODO: error handling
    }

    InstrumentProfile::freeGraal(graalProfile);

    return schedule;
}

std::vector<std::string> Schedule::getTradingVenues(std::shared_ptr<InstrumentProfile> profile) noexcept {
    if (!profile) {
        // TODO: error handling
        return {};
    }

    auto graalProfile = profile->toGraal();
    auto result = isolated::schedule::Schedule::getTradingVenues(graalProfile);
    InstrumentProfile::freeGraal(graalProfile);

    return result;
}

bool Schedule::setDefaults(const std::vector<char> &data) noexcept {
    return isolated::schedule::Schedule::setDefaults(data);
}

std::shared_ptr<Day> Schedule::getDayByTime(std::int64_t time) const noexcept {
    if (!handle_) {
        return {};
    }

    auto graalDay = isolated::schedule::Schedule::getDayByTime(handle_, time);

    if (!graalDay) {
        return {};
    }

    std::shared_ptr<Day> day{new (std::nothrow) Day{}};

    if (!day) {
        return {};
    }

    day->handle_ = graalDay;

    return day;
}

} // namespace dxfcpp