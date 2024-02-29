// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

TimeFormat::TimeFormat(void *handle) noexcept : handle_(handle){};

TimeFormat::TimeFormat(JavaObjectHandle<TimeFormat> &&handle) noexcept : handle_(std::move(handle)){};

const TimeFormat TimeFormat::DEFAULT(isolated::TimeFormat::getDefault());
const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS(DEFAULT.handle_ ? isolated::TimeFormat::withMillis(DEFAULT.handle_)
                                                                 : JavaObjectHandle<TimeFormat>{nullptr});
const TimeFormat
    TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE(DEFAULT_WITH_MILLIS.handle_
                                                      ? isolated::TimeFormat::withTimeZone(DEFAULT_WITH_MILLIS.handle_)
                                                      : JavaObjectHandle<TimeFormat>{nullptr});
const TimeFormat TimeFormat::GMT(isolated::TimeFormat::getGmt());

std::int64_t TimeFormat::parse(const std::string &value) const noexcept {
    return isolated::TimeFormat::parse(handle_, value);
}

std::string TimeFormat::format(std::int64_t timestamp) const noexcept {
    return isolated::TimeFormat::format(handle_, timestamp);
}

DXFCPP_END_NAMESPACE