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

void Candle::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    setEventSymbol(CandleSymbol::valueOf(dxfcpp::toString(graalCandle->event_symbol)));

    data_ = {
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
}

void Candle::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    graalCandle->event_symbol = createCString(eventSymbol_.value_or(CandleSymbol::NUL).toString());
    graalCandle->event_time = data_.eventTime;
    graalCandle->event_flags = data_.eventFlags;
    graalCandle->index = data_.index;
    graalCandle->count = data_.count;
    graalCandle->open = data_.open;
    graalCandle->high = data_.high;
    graalCandle->low = data_.low;
    graalCandle->close = data_.close;
    graalCandle->volume = data_.volume;
    graalCandle->vwap = data_.vwap;
    graalCandle->bid_volume = data_.bidVolume;
    graalCandle->ask_volume = data_.askVolume;
    graalCandle->imp_volatility = data_.impVolatility;
    graalCandle->open_interest = data_.openInterest;
}

void Candle::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    delete[] graalCandle->event_symbol;
}

std::shared_ptr<Candle> Candle::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        return {};
    }

    try {
        auto candle = std::make_shared<Candle>();

        candle->fillData(graalNative);

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
        getEventSymbol().toString(), TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()),
        getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(), getCount(),
        dxfcpp::toString(getOpen()), dxfcpp::toString(getHigh()), dxfcpp::toString(getLow()),
        dxfcpp::toString(getClose()), dxfcpp::toString(getVolume()), dxfcpp::toString(getVWAP()),
        dxfcpp::toString(getBidVolume()), dxfcpp::toString(getAskVolume()), dxfcpp::toString(getImpVolatility()),
        dxfcpp::toString(getOpenInterest()));
}

void *Candle::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalCandle =
        new (std::nothrow) dxfg_candle_t{.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_CANDLE}};

    if (!graalCandle) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalCandle));

    return static_cast<void *>(graalCandle);
}

void Candle::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        return;
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalCandle;
}

} // namespace dxfcpp