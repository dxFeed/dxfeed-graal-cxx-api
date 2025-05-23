// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

DXFCPP_BEGIN_NAMESPACE

StringSymbol::StringSymbol(const StringSymbol &stringSymbol) noexcept : data_{stringSymbol.data_} {
}

StringSymbol::StringSymbol(StringSymbol &&stringSymbol) noexcept : data_{std::move(stringSymbol.data_)} {
}

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

StringSymbol::StringSymbol() noexcept {
}

StringSymbol::~StringSymbol() noexcept = default;

void *StringSymbol::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("StringSymbol::toGraal()");
    }

    auto *graalSymbol = new dxfg_string_symbol_t{{STRING}, createCString(data_)};

    return static_cast<void *>(graalSymbol);
}

void StringSymbol::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("StringSymbol::freeGraal(graalNative = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    auto *graalSymbol = static_cast<dxfg_string_symbol_t *>(graalNative);

    delete[] graalSymbol->symbol;
    delete graalSymbol;
}

StringSymbol StringSymbol::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("StringSymbol::fromGraal(graalNative = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        throw InvalidArgumentException("Unable to create StringSymbol. The `graalNative` parameter is nullptr");
    }

    auto *graalSymbol = static_cast<dxfg_string_symbol_t *>(graalNative);

    return {dxfcpp::toString(graalSymbol->symbol)};
}

const std::string &StringSymbol::getData() const {
    return data_;
}

DXFCPP_END_NAMESPACE