// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include "dxfeed_graal_cpp_api/event/candle/Candle.hpp"
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

    if (eventType->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        return {};
    }

    try {
        auto graalCandle = bit_cast<dxfg_candle_t *>(graalNative);
        auto candle = std::make_shared<Candle>(CandleSymbol::valueOf(dxfcpp::toString(graalCandle->event_symbol)));

        candle->data_ = {
            .eventTime = graalCandle->event_time,
            .eventFlags = graalCandle->event_flags,
            .index = graalCandle->index,
            .count = graalCandle->count,
            .open = graalCandle->open,
            .high = graalCandle->high,
            .low = graalCandle->low,
            .close = graalCandle->close,
            .volume = graalCandle->volume,
            .vwap = graalCandle->vwap,
            .bidVolume = graalCandle->bid_volume,
            .askVolume = graalCandle->ask_volume,
            .impVolatility = graalCandle->imp_volatility,
            .openInterest = graalCandle->open_interest,
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

void *Candle::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalCandle = new (std::nothrow) dxfg_candle_t{
        .event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_QUOTE},
        .event_symbol = createCString(eventSymbol_.toString()),
        .event_time = data_.eventTime,
        .event_flags = data_.eventFlags,
        .index = data_.index,
        .count = data_.count,
        .open = data_.open,
        .high = data_.high,
        .low = data_.low,
        .close = data_.close,
        .volume = data_.volume,
        .vwap = data_.vwap,
        .bid_volume = data_.bidVolume,
        .ask_volume = data_.askVolume,
        .imp_volatility = data_.impVolatility,
        .open_interest = data_.openInterest,
    };

    if (!graalCandle) {
        // TODO: error handling
    }

    return dxfcpp::bit_cast<void *>(graalCandle);
}

void Candle::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        return;
    }

    auto graalCandle = bit_cast<dxfg_candle_t *>(graalNative);

    delete[] graalCandle->event_symbol;
    delete graalCandle;
}

} // namespace dxfcpp