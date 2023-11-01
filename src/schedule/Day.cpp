// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Day::Day(void *handle) noexcept : handle_(handle) {
}

Day::Ptr Day::create(void *handle) noexcept {
    if (!handle) {
        return {};
    }

    return std::shared_ptr<Day>(new (std::nothrow) Day(handle));
}

std::int32_t Day::getYearMonthDay() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Day::getYearMonthDay(handle_.get());
}

Day::Ptr Day::findNextDay(const DayFilter &filter) const noexcept {
    if (!handle_ || !filter.handle_) {
        return {};
    }

    return create(isolated::schedule::Day::findNextDay(handle_.get(), filter.handle_.get()));
}

std::string Day::toString() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Day::toString(handle_.get());
}

} // namespace dxfcpp