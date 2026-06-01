// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/api.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

const std::string WildcardSymbol::RESERVED_PREFIX = "*";

const WildcardSymbol WildcardSymbol::ALL{RESERVED_PREFIX};

WildcardSymbol::WildcardSymbol(const std::string &symbol) noexcept : symbol_{symbol} {
}

WildcardSymbol::WildcardSymbol(const WildcardSymbol &) noexcept = default;

WildcardSymbol::WildcardSymbol(WildcardSymbol &&) noexcept = default;

WildcardSymbol &WildcardSymbol::operator=(const WildcardSymbol &) noexcept = default;

WildcardSymbol &WildcardSymbol::operator=(WildcardSymbol &&) noexcept = default;

WildcardSymbol::WildcardSymbol() noexcept = default;

WildcardSymbol::~WildcardSymbol() noexcept = default;

const std::string &WildcardSymbol::getSymbol() const noexcept {
    return symbol_;
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
        return "WildcardSymbol{" + symbol_ + "}";
    } else {
        return symbol_;
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

DXFCPP_END_NAMESPACE