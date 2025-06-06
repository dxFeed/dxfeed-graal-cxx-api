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

const EventTypeEnum &Greeks::TYPE = EventTypeEnum::GREEKS;

void Greeks::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

    data_ = {
        .eventFlags = graalGreeks->event_flags,
        .index = graalGreeks->index,
        .price = graalGreeks->price,
        .volatility = graalGreeks->volatility,
        .delta = graalGreeks->delta,
        .gamma = graalGreeks->gamma,
        .theta = graalGreeks->theta,
        .rho = graalGreeks->rho,
        .vega = graalGreeks->vega,
    };
}

void Greeks::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

    graalGreeks->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_GREEKS;
    graalGreeks->event_flags = data_.eventFlags;
    graalGreeks->index = data_.index;
    graalGreeks->price = data_.price;
    graalGreeks->volatility = data_.volatility;
    graalGreeks->delta = data_.delta;
    graalGreeks->gamma = data_.gamma;
    graalGreeks->theta = data_.theta;
    graalGreeks->rho = data_.rho;
    graalGreeks->vega = data_.vega;
}

std::shared_ptr<Greeks> Greeks::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Greeks. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_GREEKS) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Greeks. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_GREEKS))));
    }

    auto greeks = std::make_shared<Greeks>();

    greeks->fillData(graalNative);

    return greeks;
}

std::string Greeks::toString() const {
    return fmt::format(
        "Greeks{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, volatility={}, delta={}, "
        "gamma={}, theta={}, rho={}, vega={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getVolatility()), dxfcpp::toString(getDelta()),
        dxfcpp::toString(getGamma()), dxfcpp::toString(getTheta()), dxfcpp::toString(getRho()),
        dxfcpp::toString(getVega()));
}

void *Greeks::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalGreeks = new dxfg_greeks_t{};

    fillGraalData(static_cast<void *>(graalGreeks));

    return static_cast<void *>(graalGreeks);
}

void Greeks::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_GREEKS) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Greeks's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_GREEKS))));
    }

    auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalGreeks;
}

void Greeks::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Greeks>(); other) {
        data_ = other->data_;
    }
}

DXFCPP_END_NAMESPACE