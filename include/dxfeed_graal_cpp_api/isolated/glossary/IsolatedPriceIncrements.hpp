// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../glossary/PriceIncrements.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedPriceIncrements {

// int32_t dxfg_PriceIncrements_EMPTY(graal_isolatethread_t *thread, DXFG_OUT dxfg_price_increments_t **emptyPriceIncrements);
JavaObjectHandle<PriceIncrements> EMPTY();

// int32_t dxfg_PriceIncrements_valueOf(graal_isolatethread_t *thread, const char *text, DXFG_OUT dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(const StringLikeWrapper &text);

// int32_t dxfg_PriceIncrements_valueOf2(graal_isolatethread_t *thread, double increment, DXFG_OUT dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(double increment);

// int32_t dxfg_PriceIncrements_valueOf3(graal_isolatethread_t *thread, const double *increments, int32_t size, DXFG_OUT dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(const std::vector<double> &increments);

// int32_t dxfg_PriceIncrements_getText(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, DXFG_OUT char **text);
std::string getText(const JavaObjectHandle<PriceIncrements> &priceIncrements);

// int32_t dxfg_PriceIncrements_getPriceIncrements(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, DXFG_OUT double **increments, DXFG_OUT int32_t *size);
std::vector<double> getPriceIncrements(const JavaObjectHandle<PriceIncrements> &priceIncrements);

// int32_t dxfg_PriceIncrements_getPriceIncrement(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements);

// int32_t dxfg_PriceIncrements_getPriceIncrement2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price);

// int32_t dxfg_PriceIncrements_getPriceIncrement3(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, int32_t direction, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction);

// int32_t dxfg_PriceIncrements_getPricePrecision(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, DXFG_OUT int32_t *pricePrecision);
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements);

// int32_t dxfg_PriceIncrements_getPricePrecision2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, DXFG_OUT int32_t *pricePrecision);
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price);

// int32_t dxfg_PriceIncrements_roundPrice(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price);

// int32_t dxfg_PriceIncrements_roundPrice2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, int32_t direction, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction);

// int32_t dxfg_PriceIncrements_roundPrice3(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, dxfg_rounding_mode_t roundingMode, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, RoundingMode roundingMode);

// int32_t dxfg_PriceIncrements_incrementPrice(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, int32_t direction, DXFG_OUT double *incrementedPrice);
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction);

// int32_t dxfg_PriceIncrements_incrementPrice2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements, double price, int32_t direction, double step, DXFG_OUT double *incrementedPrice);
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction, double step);

}

DXFCPP_END_NAMESPACE