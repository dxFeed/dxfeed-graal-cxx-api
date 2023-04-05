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

const PriceType PriceType::REGULAR{0, "REGULAR"};
const PriceType PriceType::INDICATIVE{1, "INDICATIVE"};
const PriceType PriceType::PRELIMINARY{2, "PRELIMINARY"};
const PriceType PriceType::FINAL{3, "FINAL"};

template <>
const std::unordered_map<PriceType::CodeType, std::reference_wrapper<const PriceType>> PriceType::ParentType::ALL{
    {PriceType::REGULAR.getCode(), std::cref(PriceType::REGULAR)},
    {PriceType::INDICATIVE.getCode(), std::cref(PriceType::INDICATIVE)},
    {PriceType::PRELIMINARY.getCode(), std::cref(PriceType::PRELIMINARY)},
    {PriceType::FINAL.getCode(), std::cref(PriceType::FINAL)},
};

}