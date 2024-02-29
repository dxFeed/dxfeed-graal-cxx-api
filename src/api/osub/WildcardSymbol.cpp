// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"

DXFCPP_BEGIN_NAMESPACE

const std::string WildcardSymbol::RESERVED_PREFIX = "*";

const WildcardSymbol WildcardSymbol::ALL{RESERVED_PREFIX};

void *WildcardSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::toGraal()");
    }

    static const dxfg_wildcard_symbol_t wildcardGraalSymbol{{WILDCARD}};

    return dxfcpp::bit_cast<void *>(&wildcardGraalSymbol);
}

void WildcardSymbol::freeGraal(void *) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::freeGraal()");
    }
}

const WildcardSymbol &WildcardSymbol::fromGraal(void *) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("WildcardSymbol::fromGraal()");
    }

    return WildcardSymbol::ALL;
}

DXFCPP_END_NAMESPACE