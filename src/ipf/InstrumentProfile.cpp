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
        .localSymbol = dxfcpp::toString(graalInstrumentProfile->local_symbol),
        .localDescription = dxfcpp::toString(graalInstrumentProfile->local_description),
        .country = dxfcpp::toString(graalInstrumentProfile->country),
        .opol = dxfcpp::toString(graalInstrumentProfile->opol),
        .exchangeData = dxfcpp::toString(graalInstrumentProfile->exchange_data),
        .exchanges = dxfcpp::toString(graalInstrumentProfile->exchanges),
        .currency = dxfcpp::toString(graalInstrumentProfile->currency),
        .baseCurrency = dxfcpp::toString(graalInstrumentProfile->base_currency),
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
        .additionalUnderlyings = dxfcpp::toString(graalInstrumentProfile->additional_underlyings),
        .mmy = dxfcpp::toString(graalInstrumentProfile->mmy),
        .expiration = graalInstrumentProfile->expiration,
        .lastTrade = graalInstrumentProfile->last_trade,
        .strike = graalInstrumentProfile->strike,
        .optionType = dxfcpp::toString(graalInstrumentProfile->option_type),
        .expirationStyle = dxfcpp::toString(graalInstrumentProfile->expiration_style),
        .settlementStyle = dxfcpp::toString(graalInstrumentProfile->settlement_style),
        .priceIncrements = dxfcpp::toString(graalInstrumentProfile->price_increments),
        .tradingHours = dxfcpp::toString(graalInstrumentProfile->trading_hours),
        .rawCustomFields =
            [](dxfg_string_list *strings) {
                std::vector<std::string> result{};

                if (!strings || strings->size == 0) {
                    return result;
                }

                result.resize(strings->size);

                for (int i = 0; i < strings->size; i++) {
                    result[i] = dxfcpp::toString(strings->elements[i]);
                }

                return result;
            }(graalInstrumentProfile->custom_fields),
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
            }(graalInstrumentProfile->custom_fields),
    };
}

void InstrumentProfile::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalInstrumentProfile = static_cast<dxfg_instrument_profile_t *>(graalNative);

    graalInstrumentProfile->type = createCString(data_.type);
    graalInstrumentProfile->symbol = createCString(data_.symbol);
    graalInstrumentProfile->description = createCString(data_.description);
    graalInstrumentProfile->local_symbol = createCString(data_.localSymbol);
    graalInstrumentProfile->local_description = createCString(data_.localDescription);
    graalInstrumentProfile->country = createCString(data_.country);
    graalInstrumentProfile->opol = createCString(data_.opol);
    graalInstrumentProfile->exchange_data = createCString(data_.exchangeData);
    graalInstrumentProfile->exchanges = createCString(data_.exchanges);
    graalInstrumentProfile->currency = createCString(data_.currency);
    graalInstrumentProfile->base_currency = createCString(data_.baseCurrency);
    graalInstrumentProfile->cfi = createCString(data_.cfi);
    graalInstrumentProfile->isin = createCString(data_.isin);
    graalInstrumentProfile->sedol = createCString(data_.sedol);
    graalInstrumentProfile->cusip = createCString(data_.cusip);
    graalInstrumentProfile->icb = data_.icb;
    graalInstrumentProfile->sic = data_.sic;
    graalInstrumentProfile->multiplier = data_.multiplier;
    graalInstrumentProfile->product = createCString(data_.product);
    graalInstrumentProfile->underlying = createCString(data_.underlying);
    graalInstrumentProfile->spc = data_.spc;
    graalInstrumentProfile->additional_underlyings = createCString(data_.additionalUnderlyings);
    graalInstrumentProfile->mmy = createCString(data_.mmy);
    graalInstrumentProfile->expiration = data_.expiration;
    graalInstrumentProfile->last_trade = data_.lastTrade;
    graalInstrumentProfile->strike = data_.strike;
    graalInstrumentProfile->option_type = createCString(data_.optionType);
    graalInstrumentProfile->expiration_style = createCString(data_.expirationStyle);
    graalInstrumentProfile->settlement_style = createCString(data_.settlementStyle);
    graalInstrumentProfile->price_increments = createCString(data_.priceIncrements);
    graalInstrumentProfile->trading_hours = createCString(data_.tradingHours);

    if (data_.rawCustomFields.empty()) {
        graalInstrumentProfile->custom_fields = nullptr;
    } else {
        graalInstrumentProfile->custom_fields = new (std::nothrow) dxfg_string_list{};
        graalInstrumentProfile->custom_fields->size = static_cast<std::int32_t>(data_.rawCustomFields.size());
        graalInstrumentProfile->custom_fields->elements =
            new (std::nothrow) const char *[data_.rawCustomFields.size()] {
                nullptr
            };

        for (int i = 0; i < graalInstrumentProfile->custom_fields->size; i++) {
            // TODO: process null-strings. <null>?
            if (!data_.rawCustomFields[i].empty()) {
                graalInstrumentProfile->custom_fields->elements[i] = createCString(data_.rawCustomFields[i]);
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
    delete[] graalInstrumentProfile->local_symbol;
    delete[] graalInstrumentProfile->local_description;
    delete[] graalInstrumentProfile->country;
    delete[] graalInstrumentProfile->opol;
    delete[] graalInstrumentProfile->exchange_data;
    delete[] graalInstrumentProfile->exchanges;
    delete[] graalInstrumentProfile->currency;
    delete[] graalInstrumentProfile->base_currency;
    delete[] graalInstrumentProfile->cfi;
    delete[] graalInstrumentProfile->isin;
    delete[] graalInstrumentProfile->sedol;
    delete[] graalInstrumentProfile->cusip;
    delete[] graalInstrumentProfile->product;
    delete[] graalInstrumentProfile->underlying;
    delete[] graalInstrumentProfile->additional_underlyings;
    delete[] graalInstrumentProfile->mmy;
    delete[] graalInstrumentProfile->option_type;
    delete[] graalInstrumentProfile->expiration_style;
    delete[] graalInstrumentProfile->settlement_style;
    delete[] graalInstrumentProfile->price_increments;
    delete[] graalInstrumentProfile->trading_hours;

    if (graalInstrumentProfile->custom_fields) {
        if (graalInstrumentProfile->custom_fields->elements && graalInstrumentProfile->custom_fields->size > 0) {
            for (int i = 0; i < graalInstrumentProfile->custom_fields->size; i++) {
                delete[] graalInstrumentProfile->custom_fields->elements[i];
            }

            delete[] graalInstrumentProfile->custom_fields->elements;
        }

        delete graalInstrumentProfile->custom_fields;
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
        // TODO: error handling [EN-8232]
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
    auto *graalInstrumentProfile = new (std::nothrow) dxfg_instrument_profile_t{};

    if (!graalInstrumentProfile) {
        // TODO: error handling [EN-8232]

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