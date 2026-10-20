// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/api.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

// ALL is constant-initialized; RESERVED_PREFIX() is created at the first call.
constinit const WildcardSymbol WildcardSymbol::ALL{"*"};

WildcardSymbol::WildcardSymbol(const WildcardSymbol &) noexcept = default;

WildcardSymbol::WildcardSymbol(WildcardSymbol &&) noexcept = default;

WildcardSymbol &WildcardSymbol::operator=(const WildcardSymbol &) noexcept = default;

WildcardSymbol &WildcardSymbol::operator=(WildcardSymbol &&) noexcept = default;

WildcardSymbol::WildcardSymbol() noexcept = default;

WildcardSymbol::~WildcardSymbol() noexcept = default;

std::string WildcardSymbol::getSymbol() const {
    return std::string{symbol_};
}

void *WildcardSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::toGraal()");
    }

    static const dxfg_wildcard_symbol_t wildcardGraalSymbol{{WILDCARD}};

    return dxfcpp::bit_cast<void *>(&wildcardGraalSymbol);
}

void WildcardSymbol::freeGraal(void *) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::freeGraal()");
    }
}

const WildcardSymbol &WildcardSymbol::fromGraal(void *) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::fromGraal()");
    }

    return ALL;
}

std::string WildcardSymbol::toString() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        return "WildcardSymbol{" + getSymbol() + "}";
    } else {
        return getSymbol();
    }
}

bool WildcardSymbol::operator==(const WildcardSymbol &wildcardSymbol) const {
    return symbol_ == wildcardSymbol.symbol_;
}

bool WildcardSymbol::operator<(const WildcardSymbol &wildcardSymbol) const {
    return symbol_ < wildcardSymbol.symbol_;
}

WildcardSymbol literals::operator""_ws(const char *, size_t) noexcept {
    return WildcardSymbol::ALL;
}

WildcardSymbol literals::operator""_wcs(const char *, size_t) noexcept {
    return WildcardSymbol::ALL;
}

const std::string &WildcardSymbol::RESERVED_PREFIX() {
    static const auto *const value = new std::string{"*"};

    return *value;
}

DXFCPP_END_NAMESPACE