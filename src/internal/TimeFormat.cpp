// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedTimeFormat.hpp"

DXFCPP_BEGIN_NAMESPACE

// The instances are constant-initialized (constinit), so they can be used in the dynamic initialization of global
// variables of other translation units. The Java objects are created at the first use.
constinit const TimeFormat TimeFormat::DEFAULT{[] {
    return isolated::internal::IsolatedTimeFormat::getDefault();
}};

constinit const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS{[] {
    return DEFAULT.getHandle() ? isolated::internal::IsolatedTimeFormat::withMillis(DEFAULT.getHandle())
                               : JavaObjectHandle<TimeFormat>{nullptr};
}};

constinit const TimeFormat TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE{[] {
    return DEFAULT_WITH_MILLIS.getHandle()
               ? isolated::internal::IsolatedTimeFormat::withTimeZone(DEFAULT_WITH_MILLIS.getHandle())
               : JavaObjectHandle<TimeFormat>{nullptr};
}};

constinit const TimeFormat TimeFormat::GMT{[] {
    return isolated::internal::IsolatedTimeFormat::getGmt();
}};

TimeFormat::~TimeFormat() noexcept {
}

const JavaObjectHandle<TimeFormat> &TimeFormat::getHandle() const {
    if (initializer_) {
        std::call_once(initialized_, [this] {
            handle_ = initializer_();
        });
    }

    return handle_;
}

std::int64_t TimeFormat::parse(const StringLike &value) const {
    return isolated::internal::IsolatedTimeFormat::parse(getHandle(), value);
}

std::string TimeFormat::format(std::int64_t timestamp) const {
    return isolated::internal::IsolatedTimeFormat::format(getHandle(), timestamp);
}

DXFCPP_END_NAMESPACE