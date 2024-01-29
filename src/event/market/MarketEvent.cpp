// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void MarketEvent::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    setEventSymbol(dxfcpp::toString(graalMarketEvent->event_symbol));
    setEventTime(graalMarketEvent->event_time);
}

void MarketEvent::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    graalMarketEvent->event_symbol = dxfcpp::createCString(getEventSymbol());
    graalMarketEvent->event_time = getEventTime();
}

void MarketEvent::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    delete[] graalMarketEvent->event_symbol;
}

} // namespace dxfcpp