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

//    auto *graalSymbol = new (std::nothrow)
//        dxfg_indexed_event_subscription_symbol_t{{INDEXED_EVENT_SUBSCRIPTION},
//                                                 dxfcpp::bit_cast<dxfg_symbol_t *>(eventSymbol_->toGraal()),
//                                                 dxfcpp::bit_cast<dxfg_indexed_event_source_t *>(source_.toGraal())};
//
//    return dxfcpp::bit_cast<void *>(graalSymbol);

    return nullptr;
}

void CandleSymbol::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

//    auto *graalSymbol = dxfcpp::bit_cast<dxfg_indexed_event_subscription_symbol_t *>(graal);
//
//    SymbolWrapper::freeGraal(graalSymbol->symbol);
//    IndexedEventSource::freeGraal(graalSymbol->source);
//
//    delete graalSymbol;
}

CandleSymbol CandleSymbol::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("CandleSymbol::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

//    auto *graalSymbol = dxfcpp::bit_cast<dxfg_indexed_event_subscription_symbol_t *>(graal);
//
//    return {SymbolWrapper::fromGraal(graalSymbol->symbol), IndexedEventSource::fromGraal(graalSymbol->source)};

    return {};
}

}