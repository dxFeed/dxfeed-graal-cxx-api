// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketEvent.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

MarketEvent::MarketEvent(const StringLike &eventSymbol) noexcept : eventSymbol_{eventSymbol} {
}

void MarketEvent::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    setEventSymbol(dxfcpp::toString(graalMarketEvent->event_symbol));
    setEventTime(graalMarketEvent->event_time);
}

void MarketEvent::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    graalMarketEvent->event_symbol = createCString(getEventSymbol());
    graalMarketEvent->event_time = getEventTime();
}

void MarketEvent::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMarketEvent = static_cast<dxfg_market_event_t *>(graalNative);

    delete[] graalMarketEvent->event_symbol;
}

void MarketEvent::assign(std::shared_ptr<EventType> event) {
    if (const auto other = event->sharedAs<MarketEvent>(); other) {
        eventSymbol_ = other->eventSymbol_;
        eventTime_ = other->eventTime_;
    }
}

const std::string &MarketEvent::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return String::NUL;
    }

    return eventSymbol_.value();
}

const std::optional<std::string> &MarketEvent::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void MarketEvent::setEventSymbol(const StringLike &eventSymbol) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    eventSymbol_ = std::string(eventSymbol);
}

std::int64_t MarketEvent::getEventTime() const noexcept {
    return eventTime_;
}

void MarketEvent::setEventTime(std::int64_t eventTime) noexcept {
    eventTime_ = eventTime;
}

DXFCPP_END_NAMESPACE