// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/glossary/IsolatedPriceIncrements.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedPriceIncrements {

// int32_t dxfg_PriceIncrements_EMPTY(graal_isolatethread_t *thread, DXFG_OUT dxfg_price_increments_t
// **emptyPriceIncrements);
JavaObjectHandle<PriceIncrements> EMPTY() {
    dxfg_price_increments_t *isolatedPriceIncrements{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_EMPTY, &isolatedPriceIncrements);

    return JavaObjectHandle<PriceIncrements>{isolatedPriceIncrements};
}

// int32_t dxfg_PriceIncrements_valueOf(graal_isolatethread_t *thread, const char *text, DXFG_OUT
// dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(const StringLikeWrapper &text) {
    dxfg_price_increments_t *isolatedPriceIncrements{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_valueOf, text.c_str(), &isolatedPriceIncrements);

    return JavaObjectHandle<PriceIncrements>{isolatedPriceIncrements};
}

// int32_t dxfg_PriceIncrements_valueOf2(graal_isolatethread_t *thread, double increment, DXFG_OUT
// dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(double increment) {
    dxfg_price_increments_t *isolatedPriceIncrements{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_valueOf2, increment, &isolatedPriceIncrements);

    return JavaObjectHandle<PriceIncrements>{isolatedPriceIncrements};
}

// int32_t dxfg_PriceIncrements_valueOf3(graal_isolatethread_t *thread, const double *increments, int32_t size, DXFG_OUT
// dxfg_price_increments_t **priceIncrements);
JavaObjectHandle<PriceIncrements> valueOf(const std::vector<double> &increments) {
    dxfg_price_increments_t *isolatedPriceIncrements{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_valueOf3, increments.data(),
                                       fitToType<std::int32_t>(increments.size()), &isolatedPriceIncrements);

    return JavaObjectHandle<PriceIncrements>{isolatedPriceIncrements};
}

// int32_t dxfg_PriceIncrements_getText(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// DXFG_OUT char **text);
std::string getText(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getText`. The "
                                       "`priceIncrements` handle is invalid");
    }

    char *text{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getText,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), &text);

    auto uText = internal::IsolatedString::toUnique(text);
    auto result = dxfcpp::toString(uText.get());

    return result;
}

// int32_t dxfg_PriceIncrements_getPriceIncrements(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, DXFG_OUT double **increments, DXFG_OUT int32_t *size);
std::vector<double> getPriceIncrements(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPriceIncrements`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double *increments{};
    std::int32_t size{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPriceIncrements,
                                   static_cast<dxfg_price_increments_t *>(priceIncrements.get()), &increments, &size);

    if (increments != nullptr) {
        return {};
    }

    DXFCPP_FINALLY([&] {
        runGraalFunctionAndThrowIfMinusOne(dxfg_free, increments);
    });

    std::vector<double> result{};

    result.reserve(size);

    for (auto i = 0; i < size; i++) {
        result.emplace_back(increments[i]);
    }

    return result;
}

// int32_t dxfg_PriceIncrements_getPriceIncrement(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPriceIncrement`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPriceIncrement,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()),
                                       &result);

    return result;
}

// int32_t dxfg_PriceIncrements_getPriceIncrement2(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, double price, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPriceIncrement2`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPriceIncrement2,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price,
                                       &result);

    return result;
}

// int32_t dxfg_PriceIncrements_getPriceIncrement3(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, double price, int32_t direction, DXFG_OUT double *priceIncrement);
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price,
                         std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPriceIncrement3`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPriceIncrement3,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, direction,
                                       &result);

    return result;
}

// int32_t dxfg_PriceIncrements_getPricePrecision(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, DXFG_OUT int32_t *pricePrecision);
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPricePrecision`. The "
                                       "`priceIncrements` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPricePrecision,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), &result);

    return result;
}

// int32_t dxfg_PriceIncrements_getPricePrecision2(graal_isolatethread_t *thread, dxfg_price_increments_t
// *priceIncrements, double price, DXFG_OUT int32_t *pricePrecision);
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_getPricePrecision2`. The "
                                       "`priceIncrements` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_getPricePrecision2,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, &result);

    return result;
}

// int32_t dxfg_PriceIncrements_roundPrice(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// double price, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_roundPrice`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_roundPrice,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, &result);

    return result;
}

// int32_t dxfg_PriceIncrements_roundPrice2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// double price, int32_t direction, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_roundPrice2`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_roundPrice2,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, direction,
                                       &result);

    return result;
}

// int32_t dxfg_PriceIncrements_roundPrice3(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// double price, dxfg_rounding_mode_t roundingMode, DXFG_OUT double *roundedPrice);
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, RoundingMode roundingMode) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_roundPrice3`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_roundPrice3,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price,
                                       static_cast<dxfg_rounding_mode_t>(roundingMode), &result);

    return result;
}

// int32_t dxfg_PriceIncrements_incrementPrice(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// double price, int32_t direction, DXFG_OUT double *incrementedPrice);
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_incrementPrice`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_incrementPrice,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, direction,
                                       &result);

    return result;
}

// int32_t dxfg_PriceIncrements_incrementPrice2(graal_isolatethread_t *thread, dxfg_price_increments_t *priceIncrements,
// double price, int32_t direction, double step, DXFG_OUT double *incrementedPrice);
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction,
                      double step) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_PriceIncrements_incrementPrice2`. The "
                                       "`priceIncrements` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_PriceIncrements_incrementPrice2,
                                       static_cast<dxfg_price_increments_t *>(priceIncrements.get()), price, direction,
                                       step, &result);

    return result;
}

} // namespace isolated::glossary::IsolatedPriceIncrements

DXFCPP_END_NAMESPACE