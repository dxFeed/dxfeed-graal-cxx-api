// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Candle::TYPE = EventTypeEnum::CANDLE;

void Candle::fillData(void *graalNative) {
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

void Candle::fillGraalData(void *graalNative) const {
    if (graalNative == nullptr) {
        return;
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    graalCandle->event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_CANDLE;
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

std::shared_ptr<Candle> Candle::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Candle. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Candle. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_CANDLE))));
    }

    auto candle = std::make_shared<Candle>();

    candle->fillData(graalNative);

    return candle;
}

void *Candle::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalCandle = new dxfg_candle_t{};

    fillGraalData(static_cast<void *>(graalCandle));

    return static_cast<void *>(graalCandle);
}

void Candle::assign(std::shared_ptr<EventType> event) {
    if (const auto other = event->sharedAs<Candle>(); other) {
        eventSymbol_ = other->eventSymbol_;
        data_ = other->data_;
    }
}

void Candle::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_CANDLE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Candle's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_CANDLE))));
    }

    auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalCandle;
}

std::string Candle::toString() const {
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

DXFCPP_END_NAMESPACE