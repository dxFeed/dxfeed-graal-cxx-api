// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/util/TimePeriod.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/util/IsolatedTimePeriod.hpp"

#include <utility>

DXFCPP_BEGIN_NAMESPACE

// The constants are constant-initialized (constinit), so they can be used in the dynamic initialization of global
// variables of other translation units. The Java objects are created at the first use.
constinit const TimePeriod TimePeriod::ZERO{&isolated::util::IsolatedTimePeriod::ZERO};
constinit const TimePeriod TimePeriod::UNLIMITED{&isolated::util::IsolatedTimePeriod::UNLIMITED};

TimePeriod TimePeriod::valueOf(std::int64_t value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

TimePeriod TimePeriod::valueOf(std::chrono::milliseconds value) {
    return valueOf(value.count());
}

TimePeriod TimePeriod::valueOf(const StringLike &value) {
    return TimePeriod(isolated::util::IsolatedTimePeriod::valueOf(value));
}

std::int64_t TimePeriod::getTime() const {
    return isolated::util::IsolatedTimePeriod::getTime(getHandle());
}

std::int32_t TimePeriod::getSeconds() const {
    return isolated::util::IsolatedTimePeriod::getSeconds(getHandle());
}

std::int64_t TimePeriod::getNanos() const {
    return isolated::util::IsolatedTimePeriod::getNanos(getHandle());
}

TimePeriod::~TimePeriod() noexcept {
}

TimePeriod::TimePeriod(JavaObjectHandle<TimePeriod> &&handle) : handle_(std::move(handle)) {
}

const JavaObjectHandle<TimePeriod> &TimePeriod::getHandle() const {
    return handle_.get();
}

DXFCPP_END_NAMESPACE
