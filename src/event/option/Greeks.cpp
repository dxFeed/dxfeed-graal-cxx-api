// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/option/Greeks.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Greeks::TYPE = EventTypeEnum::GREEKS;

void Greeks::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

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

    const auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

    graalGreeks->market_event.event_type.clazz = DXFG_EVENT_GREEKS;
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

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_GREEKS) {
        throw InvalidArgumentException(fmt::format("Unable to create Greeks. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_GREEKS)));
    }

    auto greeks = std::make_shared<Greeks>();

    greeks->fillData(graalNative);

    return greeks;
}

void *Greeks::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalGreeks = new dxfg_greeks_t{};

    fillGraalData(graalGreeks);

    return graalGreeks;
}

void Greeks::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_GREEKS) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Greeks's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_GREEKS)));
    }

    auto graalGreeks = static_cast<dxfg_greeks_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalGreeks;
}

void Greeks::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Greeks>(); other) {
        data_ = other->data_;
    }
}

Greeks::Greeks() noexcept {
}

Greeks::Greeks(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const IndexedEventSource &Greeks::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t Greeks::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask Greeks::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void Greeks::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

void Greeks::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

std::int64_t Greeks::getIndex() const noexcept {
    return data_.index;
}

void Greeks::setIndex(std::int64_t index) {
    data_.index = index;
}

std::int64_t Greeks::getTime() const noexcept {
    return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void Greeks::setTime(std::int64_t time) noexcept {
    data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                            sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                       getSequence());
}

std::int32_t Greeks::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
}

void Greeks::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

double Greeks::getPrice() const noexcept {
    return data_.price;
}

void Greeks::setPrice(double price) noexcept {
    data_.price = price;
}

double Greeks::getVolatility() const noexcept {
    return data_.volatility;
}

void Greeks::setVolatility(double volatility) noexcept {
    data_.volatility = volatility;
}

double Greeks::getDelta() const noexcept {
    return data_.delta;
}

void Greeks::setDelta(double delta) noexcept {
    data_.delta = delta;
}

double Greeks::getGamma() const noexcept {
    return data_.gamma;
}

void Greeks::setGamma(double gamma) noexcept {
    data_.gamma = gamma;
}

double Greeks::getTheta() const noexcept {
    return data_.theta;
}

void Greeks::setTheta(double theta) noexcept {
    data_.theta = theta;
}

double Greeks::getRho() const noexcept {
    return data_.rho;
}

void Greeks::setRho(double rho) noexcept {
    data_.rho = rho;
}

double Greeks::getVega() const noexcept {
    return data_.vega;
}

void Greeks::setVega(double vega) noexcept {
    data_.vega = vega;
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

DXFCPP_END_NAMESPACE