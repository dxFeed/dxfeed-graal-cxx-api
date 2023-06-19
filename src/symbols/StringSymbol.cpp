// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

StringSymbol::StringSymbol(const StringSymbol &stringSymbol) noexcept : data_{stringSymbol.data_} {}

StringSymbol::StringSymbol(StringSymbol &&stringSymbol) noexcept : data_{std::move(stringSymbol.data_)} {}

StringSymbol &StringSymbol::operator=(const StringSymbol &stringSymbol) noexcept {
    if (this == &stringSymbol) {
        return *this;
    }

    data_ = stringSymbol.data_;

    return *this;
}

StringSymbol &StringSymbol::operator=(StringSymbol &&stringSymbol) noexcept {
    if (this == &stringSymbol) {
        return *this;
    }

    data_ = std::move(stringSymbol.data_);

    return *this;
}

StringSymbol::StringSymbol() noexcept {}

StringSymbol::~StringSymbol() noexcept = default;

void *StringSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "StringSymbol::toGraal()");
    }

    auto *graalSymbol = new (std::nothrow) dxfg_string_symbol_t{{STRING}, createCString(data_)};

    return dxfcpp::bit_cast<void *>(graalSymbol);
}

void StringSymbol::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "StringSymbol::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_string_symbol_t *>(graal);

    delete[] graalSymbol->symbol;
    delete graalSymbol;
}

StringSymbol StringSymbol::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "StringSymbol::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_string_symbol_t *>(graal);

    return {graalSymbol->symbol};
}

const std::string &StringSymbol::getData() const { return data_; }

} // namespace dxfcpp