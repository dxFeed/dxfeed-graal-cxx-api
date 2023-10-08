// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void InstrumentProfile::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalInstrumentProfile = static_cast<dxfg_instrument_profile_t *>(graalNative);

    data_ = {
        .type = dxfcpp::toString(graalInstrumentProfile->type),
        .symbol = dxfcpp::toString(graalInstrumentProfile->symbol),
        .description = dxfcpp::toString(graalInstrumentProfile->description),
        .localSymbol = dxfcpp::toString(graalInstrumentProfile->localSymbol),
        .localDescription = dxfcpp::toString(graalInstrumentProfile->localDescription),
        .country = dxfcpp::toString(graalInstrumentProfile->country),
        .opol = dxfcpp::toString(graalInstrumentProfile->opol),
        .exchangeData = dxfcpp::toString(graalInstrumentProfile->exchangeData),
        .exchanges = dxfcpp::toString(graalInstrumentProfile->exchanges),
        .currency = dxfcpp::toString(graalInstrumentProfile->currency),
        .baseCurrency = dxfcpp::toString(graalInstrumentProfile->baseCurrency),
        .cfi = dxfcpp::toString(graalInstrumentProfile->cfi),
        .isin = dxfcpp::toString(graalInstrumentProfile->isin),
        .sedol = dxfcpp::toString(graalInstrumentProfile->sedol),
        .cusip = dxfcpp::toString(graalInstrumentProfile->cusip),
        .icb = graalInstrumentProfile->icb,
        .sic = graalInstrumentProfile->sic,
        .multiplier = graalInstrumentProfile->multiplier,
        .product = dxfcpp::toString(graalInstrumentProfile->product),
        .underlying = dxfcpp::toString(graalInstrumentProfile->underlying),
        .spc = graalInstrumentProfile->spc,
        .additionalUnderlyings = dxfcpp::toString(graalInstrumentProfile->additionalUnderlyings),
        .mmy = dxfcpp::toString(graalInstrumentProfile->mmy),
        .expiration = graalInstrumentProfile->expiration,
        .lastTrade = graalInstrumentProfile->lastTrade,
        .strike = graalInstrumentProfile->strike,
        .optionType = dxfcpp::toString(graalInstrumentProfile->optionType),
        .expirationStyle = dxfcpp::toString(graalInstrumentProfile->expirationStyle),
        .settlementStyle = dxfcpp::toString(graalInstrumentProfile->settlementStyle),
        .priceIncrements = dxfcpp::toString(graalInstrumentProfile->priceIncrements),
        .tradingHours = dxfcpp::toString(graalInstrumentProfile->tradingHours),
        .customFields =
            [](dxfg_string_list *strings) {
                std::unordered_map<std::string, std::string> result{};

                if (strings->size < 2) {
                    return result;
                }

                for (int i = 0; i < strings->size - 1; i += 2) {
                    auto key = dxfcpp::toString(strings->elements[i]);

                    if (key.empty()) {
                        continue;
                    }

                    result[key] = dxfcpp::toString(strings->elements[i + 1]);
                }

                return result;
            }(graalInstrumentProfile->customFields),
    };
}

void InstrumentProfile::fillGraalData(void *graalNative) const noexcept {
}

void InstrumentProfile::freeGraalData(void *graalNative) noexcept {
}

std::shared_ptr<InstrumentProfile> InstrumentProfile::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    try {
        auto instrumentProfile = std::make_shared<InstrumentProfile>();

        instrumentProfile->fillData(graalNative);

        return instrumentProfile;
    } catch (...) {
        // TODO: error handling
        return {};
    }

    return {};
}

void *InstrumentProfile::toGraal() const noexcept {
    return nullptr;
}

void InstrumentProfile::freeGraal(void *graalNative) noexcept {
}

} // namespace dxfcpp