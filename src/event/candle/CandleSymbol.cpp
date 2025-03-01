// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

void *CandleSymbol::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::toGraal()");
    }

    auto *graalSymbol = new dxfg_candle_symbol_t{{CANDLE}, createCString(symbol_)};

    return static_cast<void *>(graalSymbol);
}

void CandleSymbol::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::freeGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    auto *graalSymbol = static_cast<dxfg_candle_symbol_t *>(graalNative);

    delete[] graalSymbol->symbol;
    delete graalSymbol;
}

CandleSymbol CandleSymbol::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::fromGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return {};
    }

    auto *graalSymbol = static_cast<dxfg_candle_symbol_t *>(graalNative);

    return CandleSymbol{graalSymbol->symbol};
}

const CandleSymbol CandleSymbol::NUL{"<null>"};

CandleSymbol::CandleSymbol(const CandleSymbol &candleSymbol) noexcept {
    symbol_ = candleSymbol.symbol_;
    initTransientFields(true);
}

CandleSymbol::CandleSymbol(CandleSymbol &&candleSymbol) noexcept {
    symbol_ = std::move(candleSymbol.symbol_);
    initTransientFields(true);
}

CandleSymbol &CandleSymbol::operator=(const CandleSymbol &candleSymbol) noexcept {
    if (this == &candleSymbol) {
        return *this;
    }

    symbol_ = candleSymbol.symbol_;
    initTransientFields(true);

    return *this;
}

CandleSymbol &CandleSymbol::operator=(CandleSymbol &&candleSymbol) noexcept {
    if (this == &candleSymbol) {
        return *this;
    }

    symbol_ = std::move(candleSymbol.symbol_);
    initTransientFields(true);

    return *this;
}

DXFCPP_END_NAMESPACE