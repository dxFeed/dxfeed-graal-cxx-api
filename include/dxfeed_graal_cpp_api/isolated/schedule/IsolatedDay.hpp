// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../schedule/DayFilter.hpp"
#include "../../schedule/Day.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedDayFilter {

/* dxfg_day_filter_t* */ JavaObjectHandle<dxfcpp::DayFilter> getInstance(std::uint32_t code);

}

namespace IsolatedDay {


}

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()