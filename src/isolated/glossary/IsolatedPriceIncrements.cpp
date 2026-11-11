// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedPriceIncrements.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedPriceIncrements {

/// dxfg_v4_PriceIncrements_EMPTY
JavaObjectHandle<PriceIncrements> EMPTY() {
    return JavaObjectHandle<PriceIncrements>{runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_EMPTY)};
}

/// dxfg_v4_PriceIncrements_valueOf
JavaObjectHandle<PriceIncrements> valueOf(std::string_view text) {
    return JavaObjectHandle<PriceIncrements>{
        runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_valueOf,
                                     SdkString{text, "PriceIncrements::valueOf(text)"}.c_str())};
}

/// dxfg_v4_PriceIncrements_valueOfIncrement
JavaObjectHandle<PriceIncrements> valueOf(double increment) {
    return JavaObjectHandle<PriceIncrements>{
        runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_valueOfIncrement, increment)};
}

/// dxfg_v4_PriceIncrements_valueOfIncrements
JavaObjectHandle<PriceIncrements> valueOf(const std::vector<double> &increments) {
    // The list stays with the caller: the SDK only reads it.
    const dxfg_v4_double_list list{fitToType<std::int32_t>(increments.size()), const_cast<double *>(increments.data())};

    return JavaObjectHandle<PriceIncrements>{
        runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_valueOfIncrements, &list)};
}

/// dxfg_v4_PriceIncrements_getText
std::string getText(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_getText`. The "
                                       "`priceIncrements` handle is invalid");
    }

    auto uText = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_PriceIncrements_getText, static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get())));
    auto result = dxfcpp::toString(uText.get());

    return result;
}

/// dxfg_v4_PriceIncrements_getPriceIncrements
std::vector<double> getPriceIncrements(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_getPriceIncrements`. The "
                                       "`priceIncrements` handle is invalid");
    }

    auto *increments = runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPriceIncrements,
                                                    static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()));

    if (increments == nullptr) {
        return {};
    }

    // Frees the array of the values and the list itself.
    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_double_list_release, increments);
    });

    std::vector<double> result{};

    if (increments->size > 0) {
        result.reserve(static_cast<std::size_t>(increments->size));
    }

    for (std::int32_t i = 0; i < increments->size; i++) {
        result.emplace_back(increments->elements[i]);
    }

    return result;
}

/// dxfg_v4_PriceIncrements_getPriceIncrement
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_getPriceIncrement`. The "
                                       "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPriceIncrement,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()));
}

/// dxfg_v4_PriceIncrements_getPriceIncrementForPrice
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_PriceIncrements_getPriceIncrementForPrice`. The "
            "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPriceIncrementForPrice,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price);
}

/// dxfg_v4_PriceIncrements_getPriceIncrementForPriceAndDirection
double getPriceIncrement(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price,
                         std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_PriceIncrements_getPriceIncrementForPriceAndDirection`. The "
            "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPriceIncrementForPriceAndDirection,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price,
                                        direction);
}

/// dxfg_v4_PriceIncrements_getPricePrecision
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_getPricePrecision`. The "
                                       "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPricePrecision,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()));
}

/// dxfg_v4_PriceIncrements_getPricePrecisionForPrice
std::int32_t getPricePrecision(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_PriceIncrements_getPricePrecisionForPrice`. The "
            "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_getPricePrecisionForPrice,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price);
}

/// dxfg_v4_PriceIncrements_roundPrice
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_roundPrice`. The "
                                       "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_roundPrice,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price);
}

/// dxfg_v4_PriceIncrements_roundPriceWithDirection
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_PriceIncrements_roundPriceWithDirection`. The "
            "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_roundPriceWithDirection,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price,
                                        direction);
}

/// dxfg_v4_PriceIncrements_roundPriceWithRoundingMode
double roundPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, RoundingMode roundingMode) {
    if (!priceIncrements) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_PriceIncrements_roundPriceWithRoundingMode`. The "
            "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_roundPriceWithRoundingMode,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price,
                                        static_cast<dxfg_v4_rounding_mode_t>(roundingMode));
}

/// dxfg_v4_PriceIncrements_incrementPrice
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_incrementPrice`. The "
                                       "`priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_incrementPrice,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price,
                                        direction);
}

/// dxfg_v4_PriceIncrements_incrementPriceWithStep
double incrementPrice(const JavaObjectHandle<PriceIncrements> &priceIncrements, double price, std::int32_t direction,
                      double step) {
    if (!priceIncrements) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_PriceIncrements_incrementPriceWithStep`. "
                                       "The `priceIncrements` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_PriceIncrements_incrementPriceWithStep,
                                        static_cast<dxfg_v4_price_increments_t *>(priceIncrements.get()), price,
                                        direction, step);
}

} // namespace isolated::glossary::IsolatedPriceIncrements

DXFCPP_END_NAMESPACE
