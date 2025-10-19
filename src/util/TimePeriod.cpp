// Copyright (c) 2025 Devexperts LLC.
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

const TimePeriod TimePeriod::ZERO([] {
    return isolated::util::IsolatedTimePeriod::ZERO();
});

const TimePeriod TimePeriod::UNLIMITED([] {
    return isolated::util::IsolatedTimePeriod::UNLIMITED();
});

TimePeriod TimePeriod::valueOf(std::int64_t value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

TimePeriod TimePeriod::valueOf(const StringLikeWrapper &value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

std::int64_t TimePeriod::getTime() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::util::IsolatedTimePeriod::getTime(handle_);
}

std::int32_t TimePeriod::getSeconds() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::util::IsolatedTimePeriod::getSeconds(handle_);
}

std::int64_t TimePeriod::getNanos() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::util::IsolatedTimePeriod::getNanos(handle_);
}

TimePeriod::TimePeriod(std::function<JavaObjectHandle<TimePeriod>()> &&initializer)
    : initializer_(std::move(initializer)) {
}

TimePeriod::TimePeriod(JavaObjectHandle<TimePeriod> &&handle) : handle_(std::move(handle)) {
    initialized_ = true;
}

void TimePeriod::init() const {
    if (!initialized_) {
        handle_ = initializer_();
        initialized_ = true;
    }
}

DXFCPP_END_NAMESPACE
