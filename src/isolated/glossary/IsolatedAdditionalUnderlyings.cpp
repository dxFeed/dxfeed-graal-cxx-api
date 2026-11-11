// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedAdditionalUnderlyings.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

#include <deque>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedAdditionalUnderlyings {
/// dxfg_v4_AdditionalUnderlyings_EMPTY
JavaObjectHandle<AdditionalUnderlyings> EMPTY() {
    return JavaObjectHandle<AdditionalUnderlyings>{runGraalV4FunctionWithResult(dxfg_v4_AdditionalUnderlyings_EMPTY)};
}

/// dxfg_v4_AdditionalUnderlyings_valueOf
JavaObjectHandle<AdditionalUnderlyings> valueOf(std::string_view value) {
    return JavaObjectHandle<AdditionalUnderlyings>{
        runGraalV4FunctionWithResult(dxfg_v4_AdditionalUnderlyings_valueOf, SdkString{value}.c_str())};
}

/// dxfg_v4_AdditionalUnderlyings_valueOfMap
JavaObjectHandle<AdditionalUnderlyings> valueOf(const std::vector<std::pair<const char *, double>> &mapLikeEntries) {
    // The keys must outlive the call. SdkString can be neither copied nor moved; a deque keeps its elements in place.
    std::deque<SdkString> keys{};
    std::vector<dxfg_v4_string_to_double_map_entry_t> graalMapEntries{};

    graalMapEntries.reserve(mapLikeEntries.size());

    for (const auto &[key, value] : mapLikeEntries) {
        keys.emplace_back(key);
        graalMapEntries.push_back(dxfg_v4_string_to_double_map_entry_t{keys.back().c_str(), value});
    }

    // The list stays with the caller: the SDK only reads it.
    const dxfg_v4_string_to_double_map_entry_list list{fitToType<std::int32_t>(graalMapEntries.size()),
                                                       graalMapEntries.data()};

    return JavaObjectHandle<AdditionalUnderlyings>{
        runGraalV4FunctionWithResult(dxfg_v4_AdditionalUnderlyings_valueOfMap, &list)};
}

/// dxfg_v4_AdditionalUnderlyings_getSPCFromText
double getSPC(std::string_view text, std::string_view symbol) {
    return runGraalV4FunctionWithResult(dxfg_v4_AdditionalUnderlyings_getSPCFromText, SdkString{text}.c_str(),
                                        SdkString{symbol}.c_str());
}

/// dxfg_v4_AdditionalUnderlyings_getText
std::string getText(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_AdditionalUnderlyings_getText`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    auto uText = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_AdditionalUnderlyings_getText,
        static_cast<dxfg_v4_additional_underlyings_t *>(additionalUnderlyings.get())));
    auto result = dxfcpp::toString(uText.get());

    return result;
}

/// dxfg_v4_AdditionalUnderlyings_getMap
std::unordered_map<std::string, double> getMap(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_AdditionalUnderlyings_getMap`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    std::unordered_map<std::string, double> result{};

    auto *map = runGraalV4FunctionWithResult(
        dxfg_v4_AdditionalUnderlyings_getMap,
        static_cast<dxfg_v4_additional_underlyings_t *>(additionalUnderlyings.get()));

    if (map == nullptr) {
        return result;
    }

    // Frees the keys of the entries, the array of the entries and the list itself.
    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_string_to_double_map_entry_list_release, map);
    });

    if (map->size > 0) {
        result.reserve(static_cast<std::size_t>(map->size));
    }

    for (std::int32_t i = 0; i < map->size; i++) {
        result[dxfcpp::toString(map->elements[i].key)] = map->elements[i].value;
    }

    return result;
}

/// dxfg_v4_AdditionalUnderlyings_getSPC
double getSPC(const JavaObjectHandle<AdditionalUnderlyings> &additionalUnderlyings, std::string_view symbol) {
    if (!additionalUnderlyings) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_AdditionalUnderlyings_getSPC`. The "
                                       "`additionalUnderlyings` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_AdditionalUnderlyings_getSPC,
                                        static_cast<dxfg_v4_additional_underlyings_t *>(additionalUnderlyings.get()),
                                        SdkString{symbol}.c_str());
}

} // namespace isolated::glossary::IsolatedAdditionalUnderlyings

DXFCPP_END_NAMESPACE
