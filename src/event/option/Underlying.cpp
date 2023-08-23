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

    graalUnderlying->event_flags = data_.eventFlags;
    graalUnderlying->index = data_.index;
    graalUnderlying->volatility = data_.volatility;
    graalUnderlying->front_volatility = data_.frontVolatility;
    graalUnderlying->back_volatility = data_.backVolatility;
    graalUnderlying->call_volume = data_.callVolume;
    graalUnderlying->put_volume = data_.putVolume;
    graalUnderlying->put_call_ratio = data_.putCallRatio;
}

std::shared_ptr<Underlying> Underlying::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING) {
        return {};
    }

    try {
        auto underlying = std::make_shared<Underlying>();

        underlying->fillData(graalNative);

        return underlying;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Underlying::toString() const noexcept {
    return fmt::format(
        "Underlying{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, volatility={}, frontVolatility={}, "
        "backVolatility={}, callVolume={}, putVolume={}, putCallRatio={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getSequence(),
        dxfcpp::toString(getVolatility()), dxfcpp::toString(getFrontVolatility()),
        dxfcpp::toString(getBackVolatility()), dxfcpp::toString(getCallVolume()), dxfcpp::toString(getPutVolume()),
        dxfcpp::toString(getPutCallRatio()));
}

void *Underlying::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalUnderlying = new (std::nothrow)
        dxfg_underlying_t{.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING}}};

    if (!graalUnderlying) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalUnderlying));

    return static_cast<void *>(graalUnderlying);
}

void Underlying::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_UNDERLYING) {
        return;
    }

    auto graalUnderlying = static_cast<dxfg_underlying_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalUnderlying;
}

} // namespace dxfcpp