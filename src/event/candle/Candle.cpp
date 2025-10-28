// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cassert>
#include <dxfeed_graal_cpp_api/event/EventTypeEnum.hpp>
#include <dxfeed_graal_cpp_api/event/candle/Candle.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/internal/TimeFormat.hpp>
#include <dxfeed_graal_cpp_api/internal/utils/debug/Debug.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfg_api.h>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

void Candle::fillData(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

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

    const auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    graalCandle->event_type.clazz = DXFG_EVENT_CANDLE;
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

    const auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    delete[] graalCandle->event_symbol;
}

const EventTypeEnum &Candle::TYPE = EventTypeEnum::CANDLE;

std::shared_ptr<Candle> Candle::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Candle. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_CANDLE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(std::string("Unable to create Candle. Wrong event class ") +
                                       isolated::toString(eventType).data() + "(" + std::to_string(eventType) +
                                       ")! Expected: " + isolated::toString(DXFG_EVENT_CANDLE).data());
    }

    auto candle = std::make_shared<Candle>();

    candle->fillData(graalNative);

    return candle;
}

void *Candle::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalCandle = new dxfg_candle_t{};

    fillGraalData(graalCandle);

    return graalCandle;
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

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_CANDLE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(std::string("Unable to free Candle's Graal data. Wrong event class ") +
                                       isolated::toString(eventType).data() + "(" + std::to_string(eventType) +
                                       ")! Expected: " + isolated::toString(DXFG_EVENT_CANDLE).data());
    }

    const auto graalCandle = static_cast<dxfg_candle_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalCandle;
}

Candle::Candle(CandleSymbol eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {
}

const CandleSymbol &Candle::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return CandleSymbol::NUL;
    }

    return eventSymbol_.value();
}

const std::optional<CandleSymbol> &Candle::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void Candle::setEventSymbol(const CandleSymbol &eventSymbol) noexcept {
    eventSymbol_ = eventSymbol;
}

Candle &Candle::withEventSymbol(const CandleSymbol &eventSymbol) noexcept {
    Candle::setEventSymbol(eventSymbol);

    return *this;
}

std::int64_t Candle::getEventTime() const noexcept {
    return data_.eventTime;
}

void Candle::setEventTime(std::int64_t eventTime) noexcept {
    data_.eventTime = eventTime;
}

Candle &Candle::withEventTime(std::int64_t eventTime) noexcept {
    Candle::setEventTime(eventTime);

    return *this;
}

// IndexedEvent methods

std::int32_t Candle::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask Candle::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void Candle::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

Candle &Candle::withEventFlags(std::int32_t eventFlags) noexcept {
    Candle::setEventFlags(eventFlags);

    return *this;
}

void Candle::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

Candle &Candle::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    Candle::setEventFlags(eventFlags);

    return *this;
}

void Candle::setIndex(std::int64_t index) {
    data_.index = index;
}

Candle &Candle::withIndex(std::int64_t index) noexcept {
    Candle::setIndex(index);

    return *this;
}

// TimeSeriesEvent methods

std::int64_t Candle::getIndex() const noexcept {
    return data_.index;
}

std::int64_t Candle::getTime() const noexcept {
    return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

// Candle methods

void Candle::setTime(std::int64_t time) noexcept {
    data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                            sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                       getSequence());
}

Candle &Candle::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

std::int32_t Candle::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
}

void Candle::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

Candle &Candle::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

std::int64_t Candle::getCount() const noexcept {
    return data_.count;
}

void Candle::setCount(std::int64_t count) noexcept {
    data_.count = count;
}

Candle &Candle::withCount(std::int64_t count) noexcept {
    setCount(count);

    return *this;
}

double Candle::getOpen() const noexcept {
    return data_.open;
}

void Candle::setOpen(double open) noexcept {
    data_.open = open;
}

Candle &Candle::withOpen(double open) noexcept {
    setOpen(open);

    return *this;
}

double Candle::getHigh() const noexcept {
    return data_.high;
}

void Candle::setHigh(double high) noexcept {
    data_.high = high;
}

Candle &Candle::withHigh(double high) noexcept {
    setHigh(high);

    return *this;
}

double Candle::getLow() const noexcept {
    return data_.low;
}

void Candle::setLow(double low) noexcept {
    data_.low = low;
}

Candle &Candle::withLow(double low) noexcept {
    Candle::setLow(low);

    return *this;
}

double Candle::getClose() const noexcept {
    return data_.close;
}

void Candle::setClose(double close) noexcept {
    data_.close = close;
}

Candle &Candle::withClose(double close) noexcept {
    setClose(close);

    return *this;
}

double Candle::getVolume() const noexcept {
    return data_.volume;
}

void Candle::setVolume(double volume) noexcept {
    data_.volume = volume;
}

Candle &Candle::withVolume(double volume) noexcept {
    Candle::setVolume(volume);

    return *this;
}

double Candle::getVWAP() const noexcept {
    return data_.vwap;
}

void Candle::setVWAP(double vwap) noexcept {
    data_.vwap = vwap;
}

Candle &Candle::withVWAP(double vwap) noexcept {
    setVWAP(vwap);

    return *this;
}

double Candle::getBidVolume() const noexcept {
    return data_.bidVolume;
}

void Candle::setBidVolume(double bidVolume) noexcept {
    data_.bidVolume = bidVolume;
}

Candle &Candle::withBidVolume(double bidVolume) noexcept {
    setBidVolume(bidVolume);

    return *this;
}

double Candle::getAskVolume() const noexcept {
    return data_.askVolume;
}

void Candle::setAskVolume(double askVolume) noexcept {
    data_.askVolume = askVolume;
}

Candle &Candle::withAskVolume(double askVolume) noexcept {
    setAskVolume(askVolume);

    return *this;
}

double Candle::getImpVolatility() const noexcept {
    return data_.impVolatility;
}

void Candle::setImpVolatility(double impVolatility) {
    data_.impVolatility = impVolatility;
}

Candle &Candle::withImpVolatility(double impVolatility) noexcept {
    setImpVolatility(impVolatility);

    return *this;
}

double Candle::getOpenInterest() const noexcept {
    return data_.openInterest;
}

void Candle::setOpenInterest(double openInterest) noexcept {
    data_.openInterest = openInterest;
}

Candle &Candle::withOpenInterest(double openInterest) noexcept {
    setOpenInterest(openInterest);

    return *this;
}

// EventType methods

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