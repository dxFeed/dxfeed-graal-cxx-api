// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void MarketEvent::fillData(void *graalNative) noexcept {
    auto graalMarketEvent = bit_cast<dxfg_market_event_t *>(graalNative);

    setEventSymbol(dxfcpp::toString(graalMarketEvent->event_symbol));
    setEventTime(graalMarketEvent->event_time);
}

} // namespace dxfcpp