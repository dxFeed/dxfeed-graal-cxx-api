// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

const Side Side::UNDEFINED{0, "UNDEFINED"};
const Side Side::BUY{1, "BUY"};
const Side Side::SELL{2, "SELL"};

template <>
const std::unordered_map<Side::CodeType, std::reference_wrapper<const Side>> Side::ParentType::ALL{
    {Side::UNDEFINED.getCode(), std::cref(Side::UNDEFINED)},
    {Side::BUY.getCode(), std::cref(Side::BUY)},
    {Side::SELL.getCode(), std::cref(Side::SELL)},
};

DXFCPP_END_NAMESPACE