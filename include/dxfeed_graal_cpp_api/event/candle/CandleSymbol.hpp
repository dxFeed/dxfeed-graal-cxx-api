// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

namespace dxfcpp {

class DXFCPP_EXPORT CandleSymbol {
  private final String symbol;

  private transient String baseSymbol;

    transient CandleExchange exchange;
    transient CandlePrice price;
    transient CandleSession session;
    transient CandlePeriod period;
    transient CandleAlignment alignment;
    transient CandlePriceLevel priceLevel;
};

} // namespace dxfcpp