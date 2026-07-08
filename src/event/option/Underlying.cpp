// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/option/Underlying.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Underlying::TYPE = EventTypeEnum::UNDERLYING;

void Underlying::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    data_ = {
        .eventFlags = graalUnderlying->event_flags,
        .index = graalUnderlying->index,
        .volatility = graalUnderlying->volatility,
        .frontVolatility = graalUnderlying->front_volatility,
        .backVolatility = graalUnderlying->back_volatility,
        .callVolume = graalUnderlying->call_volume,
        .putVolume = graalUnderlying->put_volume,
        .putCallRatio = graalUnderlying->put_call_ratio,
    };
}

void Underlying::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    graalUnderlying->market_event.event_type.clazz = DXFG_EVENT_UNDERLYING;
    graalUnderlying->event_flags = data_.eventFlags;
    graalUnderlying->index = data_.index;
    graalUnderlying->volatility = data_.volatility;
    graalUnderlying->front_volatility = data_.frontVolatility;
    graalUnderlying->back_volatility = data_.backVolatility;
    graalUnderlying->call_volume = data_.callVolume;
    graalUnderlying->put_volume = data_.putVolume;
    graalUnderlying->put_call_ratio = data_.putCallRatio;
}

std::shared_ptr<Underlying> Underlying::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Underlying. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_UNDERLYING) {
        throw InvalidArgumentException(fmt::format("Unable to create Underlying. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_UNDERLYING)));
    }

    auto underlying = std::make_shared<Underlying>();

    underlying->fillData(graalNative);

    return underlying;
}

void *Underlying::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalUnderlying = new dxfg_underlying_t{};

    fillGraalData(graalUnderlying);

    return graalUnderlying;
}

void Underlying::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_UNDERLYING) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Underlying's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_UNDERLYING)));
    }

    const auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalUnderlying;
}

void Underlying::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Underlying>(); other) {
        data_ = other->data_;
    }
}

Underlying::Underlying() noexcept {
}

Underlying::Underlying(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const IndexedEventSource &Underlying::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t Underlying::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask Underlying::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void Underlying::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

void Underlying::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

std::int64_t Underlying::getIndex() const noexcept {
    return data_.index;
}

void Underlying::setIndex(std::int64_t index) {
    data_.index = index;
}

std::int64_t Underlying::getTime() const noexcept {
    return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void Underlying::setTime(std::int64_t time) noexcept {
    data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                            sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                       getSequence());
}

std::int32_t Underlying::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
}

void Underlying::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

double Underlying::getVolatility() const noexcept {
    return data_.volatility;
}

void Underlying::setVolatility(double volatility) noexcept {
    data_.volatility = volatility;
}

double Underlying::getFrontVolatility() const noexcept {
    return data_.frontVolatility;
}

void Underlying::setFrontVolatility(double frontVolatility) noexcept {
    data_.frontVolatility = frontVolatility;
}

double Underlying::getBackVolatility() const noexcept {
    return data_.backVolatility;
}

void Underlying::setBackVolatility(double backVolatility) noexcept {
    data_.backVolatility = backVolatility;
}

double Underlying::getCallVolume() const noexcept {
    return data_.callVolume;
}

void Underlying::setCallVolume(double callVolume) noexcept {
    data_.callVolume = callVolume;
}

double Underlying::getPutVolume() const noexcept {
    return data_.putVolume;
}

void Underlying::setPutVolume(double putVolume) noexcept {
    data_.putVolume = putVolume;
}

double Underlying::getOptionVolume() const noexcept {
    if (std::isnan(data_.putVolume)) {
        return data_.callVolume;
    }

    if (std::isnan(data_.callVolume)) {
        return data_.putVolume;
    }

    return data_.putVolume + data_.callVolume;
}

double Underlying::getPutCallRatio() const noexcept {
    return data_.putCallRatio;
}

void Underlying::setPutCallRatio(double putCallRatio) noexcept {
    data_.putCallRatio = putCallRatio;
}

std::string Underlying::toString() const {
    return fmt::format(
        "Underlying{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, volatility={}, frontVolatility={}, "
        "backVolatility={}, callVolume={}, putVolume={}, putCallRatio={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
        dxfcpp::toString(getVolatility()), dxfcpp::toString(getFrontVolatility()),
        dxfcpp::toString(getBackVolatility()), dxfcpp::toString(getCallVolume()), dxfcpp::toString(getPutVolume()),
        dxfcpp::toString(getPutCallRatio()));
}

DXFCPP_END_NAMESPACE