// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/glossary/IsolatedAdditionalUnderlyings.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedAdditionalUnderlyings {
// dxfg_AdditionalUnderlyings_EMPTY
JavaObjectHandle<AdditionalUnderlyings> EMPTY() {
    dxfg_additional_underlyings_t *isolatedAdditionalUnderlyings{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_AdditionalUnderlyings_EMPTY, &isolatedAdditionalUnderlyings);

    return JavaObjectHandle<AdditionalUnderlyings>{isolatedAdditionalUnderlyings};
}

// int32_t dxfg_AdditionalUnderlyings_valueOf(graal_isolatethread_t *thread, const char *text, DXFG_OUT
// dxfg_additional_underlyings_t **additionalUnderlyings);
JavaObjectHandle<AdditionalUnderlyings> valueOf(const StringLike &value) {
    dxfg_additional_underlyings_t *isolatedAdditionalUnderlyings{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_AdditionalUnderlyings_valueOf, value.c_str(),
                                       &isolatedAdditionalUnderlyings);

    return JavaObjectHandle<AdditionalUnderlyings>{isolatedAdditionalUnderlyings};
}

// int32_t dxfg_AdditionalUnderlyings_valueOf2(graal_isolatethread_t *thread, const dxfg_string_to_double_map_entry_t
// *mapEntries, int32_t size, DXFG_OUT dxfg_additional_underlyings_t **additionalUnderlyings);
JavaObjectHandle<AdditionalUnderlyings> valueOf(const std::vector<std::pair<const char *, double>> &mapLikeEntries) {
    std::vector<dxfg_string_to_double_map_entry_t> graalMapEntries{};
    dxfg_additional_underlyings_t *isolatedAdditionalUnderlyings{};

    graalMapEntries.reserve(mapLikeEntries.size());

    for (const auto &[key, value] : mapLikeEntries) {
        graalMapEntries.push_back(dxfg_string_to_double_map_entry_t{key, value});
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_AdditionalUnderlyings_valueOf2, graalMapEntries.data(),
                                       fitToType<std::int32_t>(graalMapEntries.size()), &isolatedAdditionalUnderlyings);

    return JavaObjectHandle<AdditionalUnderlyings>{isolatedAdditionalUnderlyings};
}

// int32_t dxfg_AdditionalUnderlyings_getSPC(graal_isolatethread_t *thread, const char *text, const char *symbol,
// DXFG_OUT double *spc);
double getSPC(const StringLike &text, const StringLike &symbol) {
    double spc{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_AdditionalUnderlyings_getSPC, text.c_str(), symbol.c_str(), &spc);

    return spc;
}

// int32_t dxfg_AdditionalUnderlyings_getText(graal_isolatethread_t *thread, dxfg_additional_underlyings_t
// *additionalUnderlyings, DXFG_OUT char **text);
std::string getText(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_AdditionalUnderlyings_getText`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    char *text{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_AdditionalUnderlyings_getText,
                                       static_cast<dxfg_additional_underlyings_t *>(additionalUnderlyings.get()),
                                       &text);

    auto uText = internal::IsolatedString::toUnique(text);
    auto result = dxfcpp::toString(uText.get());

    return result;
}

// int32_t dxfg_AdditionalUnderlyings_getMap(graal_isolatethread_t *thread, dxfg_additional_underlyings_t
// *additionalUnderlyings, DXFG_OUT dxfg_string_to_double_map_entry_t **mapEntries, DXFG_OUT int32_t *size);
std::unordered_map<std::string, double> getMap(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_AdditionalUnderlyings_getMap`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    std::unordered_map<std::string, double> result{};

    dxfg_string_to_double_map_entry_t *mapEntries{};
    std::int32_t size{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_AdditionalUnderlyings_getMap,
                                           static_cast<dxfg_additional_underlyings_t *>(additionalUnderlyings.get()),
                                           &mapEntries, &size);

    if (mapEntries == nullptr) {
        return result;
    }

    result.reserve(size);

    for (std::int32_t i = 0; i < size; i++) {
        result[dxfcpp::toString(mapEntries[i].key)] = mapEntries[i].value;
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_free_string_to_double_map_entries, mapEntries, size);

    return result;
}

// int32_t dxfg_AdditionalUnderlyings_getSPC2(graal_isolatethread_t *thread, dxfg_additional_underlyings_t
// *additionalUnderlyings, const char *symbol, DXFG_OUT double *spc);
double getSPC(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings, const StringLike &symbol) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_AdditionalUnderlyings_getSPC2`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    double result{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_AdditionalUnderlyings_getSPC2,
                                           static_cast<dxfg_additional_underlyings_t *>(additionalUnderlyings.get()),
                                           symbol.c_str(), &result);

    return result;
}

} // namespace isolated::glossary::IsolatedAdditionalUnderlyings

DXFCPP_END_NAMESPACE