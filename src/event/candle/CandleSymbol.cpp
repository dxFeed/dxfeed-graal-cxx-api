// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void *CandleSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::toGraal()");
    }

    auto *graalSymbol = new (std::nothrow) dxfg_candle_symbol_t{{CANDLE}, createCString(symbol_)};

    return dxfcpp::bit_cast<void *>(graalSymbol);
}

void CandleSymbol::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_candle_symbol_t *>(graal);

    delete[] graalSymbol->symbol;
    delete graalSymbol;
}

CandleSymbol CandleSymbol::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_candle_symbol_t *>(graal);

    return CandleSymbol{graalSymbol->symbol};
}

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

} // namespace dxfcpp