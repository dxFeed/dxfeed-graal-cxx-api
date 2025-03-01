// Copyright (c) 2025 Devexperts LLC.
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

    auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    graalUnderlying->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING;
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

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Underlying. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING))));
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
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalUnderlying = new dxfg_underlying_t{};

    fillGraalData(static_cast<void *>(graalUnderlying));

    return static_cast<void *>(graalUnderlying);
}

void Underlying::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Underlying's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING))));
    }

    auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalUnderlying;
}

void Underlying::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Underlying>(); other) {
        data_ = other->data_;
    }
}

DXFCPP_END_NAMESPACE