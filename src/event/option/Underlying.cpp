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

void Underlying::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

DXFCPP_END_NAMESPACE