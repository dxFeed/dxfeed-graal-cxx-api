// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/event/candle/CandleExchange.hpp"

DXFCPP_BEGIN_NAMESPACE

const CandleExchange CandleExchange::COMPOSITE{'\0'};
const CandleExchange CandleExchange::DEFAULT = COMPOSITE;

DXFCPP_END_NAMESPACE