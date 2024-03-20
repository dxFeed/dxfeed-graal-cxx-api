// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/internal/TimeFormat.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedTimeFormat.hpp>

DXFCPP_BEGIN_NAMESPACE

TimeFormat::TimeFormat(void *handle) : handle_(handle){};

TimeFormat::TimeFormat(JavaObjectHandle<TimeFormat> &&handle) : handle_(std::move(handle)){};

const TimeFormat TimeFormat::DEFAULT(isolated::internal::IsolatedTimeFormat::getDefault());
const TimeFormat
    TimeFormat::DEFAULT_WITH_MILLIS(DEFAULT.handle_
                                        ? isolated::internal::IsolatedTimeFormat::withMillis(DEFAULT.handle_)
                                        : JavaObjectHandle<TimeFormat>{nullptr});
const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE(
    DEFAULT_WITH_MILLIS.handle_ ? isolated::internal::IsolatedTimeFormat::withTimeZone(DEFAULT_WITH_MILLIS.handle_)
                                : JavaObjectHandle<TimeFormat>{nullptr});
const TimeFormat TimeFormat::GMT(isolated::internal::IsolatedTimeFormat::getGmt());

std::int64_t TimeFormat::parse(const std::string &value) const {
    return isolated::internal::IsolatedTimeFormat::parse(handle_, value);
}

std::string TimeFormat::format(std::int64_t timestamp) const {
    return isolated::internal::IsolatedTimeFormat::format(handle_, timestamp);
}

DXFCPP_END_NAMESPACE