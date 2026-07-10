// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/symbols/StringSymbol.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"

#include <dxfg_api.h>

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

StringSymbol::~StringSymbol() noexcept {

}

StringSymbol::StringSymbol(const char *chars) noexcept : StringSymbol() { // NOLINT(*-explicit-constructor)
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("StringSymbol(chars = " + toStringAny(chars) + ")");
    }

    data_ = std::string(chars);
}

StringSymbol::StringSymbol(std::string_view stringView) noexcept : StringSymbol() { // NOLINT(*-explicit-constructor)
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("StringSymbol(stringView = " + toStringAny(stringView) + ")");
    }

    data_ = std::string(stringView);
}

StringSymbol::StringSymbol(std::string string) noexcept : StringSymbol() { // NOLINT(*-explicit-constructor)
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("StringSymbol(string = " + toStringAny(string) + ")");
    }

    data_ = std::move(string);
}

void *StringSymbol::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("StringSymbol::toGraal()");
    }

    auto *graalSymbol = new dxfg_string_symbol_t{{STRING}, createCString(data_)};

    return graalSymbol;
}

void StringSymbol::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("StringSymbol::fromGraal(graalNative = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        throw InvalidArgumentException("Unable to create StringSymbol. The `graalNative` parameter is nullptr");
    }

    const auto *graalSymbol = static_cast<dxfg_string_symbol_t *>(graalNative);

    return {dxfcpp::toString(graalSymbol->symbol)};
}

std::string StringSymbol::toString() const { // NOLINT(*-use-nodiscard)
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        return "StringSymbol{" + data_ + "}";
    } else {
        return data_;
    }
}

const std::string &StringSymbol::getData() const {
    return data_;
}

bool StringSymbol::operator==(const StringSymbol &stringSymbol) const {
    return getData() == stringSymbol.getData();
}

bool StringSymbol::operator<(const StringSymbol &stringSymbol) const {
    return getData() < stringSymbol.getData();
}

DXFCPP_END_NAMESPACE