// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedDay.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/* dxfg_day_filter_t* */ JavaObjectHandle<dxfcpp::DayFilter> getInstance(std::uint32_t code) {
    return JavaObjectHandle<dxfcpp::DayFilter>{
        runGraalFunctionAndThrowIfNullptr(dxfg_DayFilter_getInstance, static_cast<dxfg_day_filter_prepare_t>(code))};
}

} // namespace IsolatedDayFilter

namespace IsolatedDay {}

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE