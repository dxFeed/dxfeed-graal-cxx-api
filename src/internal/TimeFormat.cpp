// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

TimeFormat::TimeFormat(void *handle) noexcept : handle_(handle){};

const TimeFormat TimeFormat::DEFAULT(isolated::TimeFormat::getDefault());
const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS(DEFAULT.handle_
                                                     ? isolated::TimeFormat::withMillis(DEFAULT.handle_.get())
                                                     : nullptr);
const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE(
    DEFAULT_WITH_MILLIS.handle_ ? isolated::TimeFormat::withTimeZone(DEFAULT_WITH_MILLIS.handle_.get()) : nullptr);
const TimeFormat TimeFormat::GMT(isolated::TimeFormat::getGmt());

std::int64_t TimeFormat::parse(const std::string &value) const noexcept {
    if (!handle_) {
        return -1;
    }

    return isolated::TimeFormat::parse(handle_.get(), value);
}

std::string TimeFormat::format(std::int64_t timestamp) const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::TimeFormat::format(handle_.get(), timestamp);
}

} // namespace dxfcpp