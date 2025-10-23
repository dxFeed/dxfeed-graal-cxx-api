// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/internal/TimeFormat.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedTimeFormat.hpp>

DXFCPP_BEGIN_NAMESPACE

const TimeFormat TimeFormat::DEFAULT([] {
    return isolated::internal::IsolatedTimeFormat::getDefault();
});

const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS([] {
    return DEFAULT.getHandle() ? isolated::internal::IsolatedTimeFormat::withMillis(DEFAULT.getHandle())
                               : JavaObjectHandle<TimeFormat>{nullptr};
});

const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE([] {
    return DEFAULT_WITH_MILLIS.getHandle()
               ? isolated::internal::IsolatedTimeFormat::withTimeZone(DEFAULT_WITH_MILLIS.getHandle())
               : JavaObjectHandle<TimeFormat>{nullptr};
});

const TimeFormat TimeFormat::GMT([] {
    return isolated::internal::IsolatedTimeFormat::getGmt();
});

TimeFormat::TimeFormat(std::function<JavaObjectHandle<TimeFormat>()> &&initializer)
    : initializer_(std::move(initializer)) {};

void TimeFormat::init() const {
    if (!initialized_) {
        handle_ = initializer_();
        initialized_ = true;
    }
}

const JavaObjectHandle<TimeFormat> &TimeFormat::getHandle() const {
    std::lock_guard lock(mtx_);

    init();

    return handle_;
}

std::int64_t TimeFormat::parse(const StringLikeWrapper &value) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::internal::IsolatedTimeFormat::parse(handle_, value);
}

std::string TimeFormat::format(std::int64_t timestamp) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::internal::IsolatedTimeFormat::format(handle_, timestamp);
}

DXFCPP_END_NAMESPACE