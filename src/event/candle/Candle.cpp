// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Candle::TYPE = EventTypeEnum::CANDLE;

std::shared_ptr<Candle> Candle::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_CANDLE) {
        return {};
    }

    try {
        auto graalCandle = bit_cast<dxfg_candle_t *>(graalNative);
        auto candle = std::make_shared<Candle>(CandleSymbol::valueOf(dxfcpp::toString(graalCandle->event_symbol)));

        candle->data_ = {
            graalCandle->event_time,     graalCandle->event_flags,   graalCandle->index,      graalCandle->count,
            graalCandle->open,           graalCandle->high,          graalCandle->low,        graalCandle->close,
            graalCandle->volume,         graalCandle->vwap,          graalCandle->bid_volume, graalCandle->ask_volume,
            graalCandle->imp_volatility, graalCandle->open_interest,
        };

        return candle;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Candle::toString() const noexcept {
    return fmt::format(
        "Candle{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, count={}, open={}, high={}, low={}, "
        "close={}, volume={}, vwap={}, bidVolume={}, askVolume={}, impVolatility={}, openInterest={}}}",
        getEventSymbol().toString(), formatTimeStampWithMillis(getEventTime()), getEventFlags().getMask(),
        formatTimeStampWithMillis(getTime()), getSequence(), getCount(), dxfcpp::toString(getOpen()),
        dxfcpp::toString(getHigh()), dxfcpp::toString(getLow()), dxfcpp::toString(getClose()),
        dxfcpp::toString(getVolume()), dxfcpp::toString(getVWAP()), dxfcpp::toString(getBidVolume()),
        dxfcpp::toString(getAskVolume()), dxfcpp::toString(getImpVolatility()), dxfcpp::toString(getOpenInterest()));
}

} // namespace dxfcpp