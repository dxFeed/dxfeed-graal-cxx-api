// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

const TimePeriod TimePeriod::ZERO(isolated::util::IsolatedTimePeriod::ZERO());

const TimePeriod TimePeriod::UNLIMITED(isolated::util::IsolatedTimePeriod::UNLIMITED());

TimePeriod TimePeriod::valueOf(std::int64_t value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

TimePeriod TimePeriod::valueOf(const StringLikeWrapper &value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

std::int64_t TimePeriod::getTime() const {
    return isolated::util::IsolatedTimePeriod::getTime(handle_);
}

std::int32_t TimePeriod::getSeconds() const {
    return isolated::util::IsolatedTimePeriod::getSeconds(handle_);
}

std::int64_t TimePeriod::getNanos() const {
    return isolated::util::IsolatedTimePeriod::getNanos(handle_);
}

TimePeriod::TimePeriod(JavaObjectHandle<TimePeriod> &&handle) : handle_(std::move(handle)) {
}

DXFCPP_END_NAMESPACE
