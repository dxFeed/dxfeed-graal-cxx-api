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
        .rawCustomFields = [](dxfg_string_list *strings) {
            std::vector<std::string> result{};

            if (!strings || strings->size == 0) {
                return result;
            }

            result.resize(strings->size);

            for (int i = 0; i < strings->size; i ++) {
                result[i] = dxfcpp::toString(strings->elements[i]);
            }

            return result;
        }(graalInstrumentProfile->customFields),
        .customFields =
            [](dxfg_string_list *strings) {
                std::unordered_map<std::string, std::string> result{};

                if (!strings || strings->size < 2) {
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
    if (graalNative == nullptr) {
        return;
    }

    auto graalInstrumentProfile = static_cast<dxfg_instrument_profile_t *>(graalNative);

    graalInstrumentProfile->type = createCString(data_.type);
    graalInstrumentProfile->symbol = createCString(data_.symbol);
    graalInstrumentProfile->description = createCString(data_.symbol);
    graalInstrumentProfile->localSymbol = createCString(data_.symbol);
    graalInstrumentProfile->localDescription = createCString(data_.symbol);
    graalInstrumentProfile->country = createCString(data_.symbol);
    graalInstrumentProfile->opol = createCString(data_.symbol);
    graalInstrumentProfile->exchangeData = createCString(data_.symbol);
    graalInstrumentProfile->exchanges = createCString(data_.symbol);
    graalInstrumentProfile->currency = createCString(data_.symbol);
    graalInstrumentProfile->baseCurrency = createCString(data_.symbol);
    graalInstrumentProfile->cfi = createCString(data_.symbol);
    graalInstrumentProfile->isin = createCString(data_.symbol);
    graalInstrumentProfile->sedol = createCString(data_.symbol);
    graalInstrumentProfile->cusip = createCString(data_.symbol);
    graalInstrumentProfile->icb = data_.icb;
    graalInstrumentProfile->sic = data_.sic;
    graalInstrumentProfile->multiplier = data_.multiplier;
    graalInstrumentProfile->product = createCString(data_.symbol);
    graalInstrumentProfile->underlying = createCString(data_.symbol);
    graalInstrumentProfile->spc = data_.spc;
    graalInstrumentProfile->additionalUnderlyings = createCString(data_.symbol);
    graalInstrumentProfile->mmy = createCString(data_.symbol);
    graalInstrumentProfile->expiration = data_.expiration;
    graalInstrumentProfile->lastTrade = data_.lastTrade;
    graalInstrumentProfile->strike = data_.strike;
    graalInstrumentProfile->optionType = createCString(data_.symbol);
    graalInstrumentProfile->expirationStyle = createCString(data_.symbol);
    graalInstrumentProfile->settlementStyle = createCString(data_.symbol);
    graalInstrumentProfile->priceIncrements = createCString(data_.symbol);
    graalInstrumentProfile->tradingHours = createCString(data_.symbol);

    if (data_.rawCustomFields.empty()) {
        graalInstrumentProfile->customFields = nullptr;
    } else {
        graalInstrumentProfile->customFields = new (std::nothrow) dxfg_string_list{};
        graalInstrumentProfile->customFields->size = static_cast<std::int32_t>(data_.rawCustomFields.size());
        graalInstrumentProfile->customFields->elements = new (std::nothrow) const char *[data_.rawCustomFields.size()] {
            nullptr
        };

        for (int i = 0; i < graalInstrumentProfile->customFields->size; i++) {
            // TODO: process null-strings. <null>?
            if (!data_.rawCustomFields[i].empty()) {
                graalInstrumentProfile->customFields->elements[i] = createCString(data_.rawCustomFields[i]);
            }
        }
    }
}

void InstrumentProfile::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }
    
    auto graalInstrumentProfile = static_cast<dxfg_instrument_profile_t *>(graalNative);

    delete[] graalInstrumentProfile->type;
    delete[] graalInstrumentProfile->symbol;
    delete[] graalInstrumentProfile->description;
    delete[] graalInstrumentProfile->localSymbol;
    delete[] graalInstrumentProfile->localDescription;
    delete[] graalInstrumentProfile->country;
    delete[] graalInstrumentProfile->opol;
    delete[] graalInstrumentProfile->exchangeData;
    delete[] graalInstrumentProfile->exchanges;
    delete[] graalInstrumentProfile->currency;
    delete[] graalInstrumentProfile->baseCurrency;
    delete[] graalInstrumentProfile->cfi;
    delete[] graalInstrumentProfile->isin;
    delete[] graalInstrumentProfile->sedol;
    delete[] graalInstrumentProfile->cusip;
    delete[] graalInstrumentProfile->product;
    delete[] graalInstrumentProfile->underlying;
    delete[] graalInstrumentProfile->additionalUnderlyings;
    delete[] graalInstrumentProfile->mmy;
    delete[] graalInstrumentProfile->optionType;
    delete[] graalInstrumentProfile->expirationStyle;
    delete[] graalInstrumentProfile->settlementStyle;
    delete[] graalInstrumentProfile->priceIncrements;
    delete[] graalInstrumentProfile->tradingHours;

    if (graalInstrumentProfile->customFields) {
        if (graalInstrumentProfile->customFields->elements && graalInstrumentProfile->customFields->size > 0) {
            for (int i = 0; i < graalInstrumentProfile->customFields->size; i++) {
                delete[] graalInstrumentProfile->customFields->elements[i];
            }

            delete[] graalInstrumentProfile->customFields->elements;
        }

        delete graalInstrumentProfile->customFields;
    }
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

std::vector<std::shared_ptr<InstrumentProfile>> InstrumentProfile::fromGraalList(void *graalList) noexcept {
    using ListType = dxfg_instrument_profile_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<std::shared_ptr<InstrumentProfile>> result{};

    auto list = dxfcpp::bit_cast<ListType *>(graalList);

    if (list->size <= 0 || list->elements == nullptr) {
        return result;
    }

    for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
        if (list->elements[elementIndex]) {
            result.emplace_back(InstrumentProfile::fromGraal(dxfcpp::bit_cast<void *>(list->elements[elementIndex])));
        }
    }

    return result;
}

void *InstrumentProfile::toGraal() const noexcept {
    auto *graalInstrumentProfile =
        new (std::nothrow) dxfg_instrument_profile_t{};

    if (!graalInstrumentProfile) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalInstrumentProfile));

    return static_cast<void *>(graalInstrumentProfile);
}

void InstrumentProfile::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    auto graalInstrumentProfile = static_cast<dxfg_instrument_profile_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalInstrumentProfile;
}

} // namespace dxfcpp