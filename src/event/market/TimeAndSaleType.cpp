// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

const TimeAndSaleType TimeAndSaleType::NEW{0, "NEW"};
const TimeAndSaleType TimeAndSaleType::CORRECTION{1, "CORRECTION"};
const TimeAndSaleType TimeAndSaleType::CANCEL{2, "CANCEL"};

template <>
const std::unordered_map<TimeAndSaleType::CodeType, std::reference_wrapper<const TimeAndSaleType>>
    TimeAndSaleType::ParentType::ALL{
        {TimeAndSaleType::NEW.getCode(), std::cref(TimeAndSaleType::NEW)},
        {TimeAndSaleType::CORRECTION.getCode(), std::cref(TimeAndSaleType::CORRECTION)},
        {TimeAndSaleType::CANCEL.getCode(), std::cref(TimeAndSaleType::CANCEL)},
    };

} // namespace dxfcpp