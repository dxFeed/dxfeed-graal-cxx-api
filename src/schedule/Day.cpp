// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Day::Day() noexcept : handle_{} {
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

    auto graalDay = isolated::schedule::Day::findNextDay(handle_.get(), filter.handle_.get());

    if (!graalDay) {
        return {};
    }

    std::shared_ptr<Day> day{new (std::nothrow) Day{}};

    if (!day) {
        return {};
    }

    day->handle_ = JavaObjectHandle<Day>(graalDay);

    return day;
}

} // namespace dxfcpp