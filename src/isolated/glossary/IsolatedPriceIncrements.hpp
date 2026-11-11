// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

#include "../../../include/dxfeed_graal_cpp_api/glossary/PriceIncrements.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedPriceIncrements {

/// dxfg_v4_PriceIncrements_EMPTY
/* dxfg_v4_price_increments_t* */ JavaObjectHandle<PriceIncrements> EMPTY();

/// dxfg_v4_PriceIncrements_valueOf
/* dxfg_v4_price_increments_t* */ JavaObjectHandle<PriceIncrements> valueOf(std::string_view text);

/// dxfg_v4_PriceIncrements_valueOfIncrement
/* dxfg_v4_price_increments_t* */ JavaObjectHandle<PriceIncrements> valueOf(double increment);

/// dxfg_v4_PriceIncrements_valueOfIncrements
/* dxfg_v4_price_increments_t* */ JavaObjectHandle<PriceIncrements> valueOf(const std::vector<double> &increments);

/// dxfg_v4_PriceIncrements_getText
std::string getText(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements);

/// dxfg_v4_PriceIncrements_getPriceIncrements
std::vector<double>
getPriceIncrements(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements);

/// dxfg_v4_PriceIncrements_getPriceIncrement
double getPriceIncrement(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements);

/// dxfg_v4_PriceIncrements_getPriceIncrementForPrice
double getPriceIncrement(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                         double price);

/// dxfg_v4_PriceIncrements_getPriceIncrementForPriceAndDirection
double getPriceIncrement(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                         double price, std::int32_t direction);

/// dxfg_v4_PriceIncrements_getPricePrecision
std::int32_t
getPricePrecision(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements);

/// dxfg_v4_PriceIncrements_getPricePrecisionForPrice
std::int32_t
getPricePrecision(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                  double price);

/// dxfg_v4_PriceIncrements_roundPrice
double roundPrice(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                  double price);

/// dxfg_v4_PriceIncrements_roundPriceWithDirection
double roundPrice(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                  double price, std::int32_t direction);

/// dxfg_v4_PriceIncrements_roundPriceWithRoundingMode
double roundPrice(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                  double price, RoundingMode roundingMode);

/// dxfg_v4_PriceIncrements_incrementPrice
double incrementPrice(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                      double price, std::int32_t direction);

/// dxfg_v4_PriceIncrements_incrementPriceWithStep
double incrementPrice(/* dxfg_v4_price_increments_t* */ const JavaObjectHandle<PriceIncrements> &priceIncrements,
                      double price, std::int32_t direction, double step);

} // namespace isolated::glossary::IsolatedPriceIncrements

DXFCPP_END_NAMESPACE
