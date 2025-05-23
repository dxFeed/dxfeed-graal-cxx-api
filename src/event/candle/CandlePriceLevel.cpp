// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

const CandlePriceLevel CandlePriceLevel::DEFAULT{math::NaN};

const std::string CandlePriceLevel::ATTRIBUTE_KEY{"pl"};

DXFCPP_END_NAMESPACE