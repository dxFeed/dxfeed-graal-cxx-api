// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/option/TheoPrice.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &TheoPrice::TYPE = EventTypeEnum::THEO_PRICE;

void TheoPrice::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    data_ = {
        .eventFlags = graalTheoPrice->event_flags,
        .index = graalTheoPrice->index,
        .price = graalTheoPrice->price,
        .underlyingPrice = graalTheoPrice->underlying_price,
        .delta = graalTheoPrice->delta,
        .gamma = graalTheoPrice->gamma,
        .dividend = graalTheoPrice->dividend,
        .interest = graalTheoPrice->interest,
    };
}

void TheoPrice::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    graalTheoPrice->market_event.event_type.clazz = DXFG_EVENT_THEO_PRICE;
    graalTheoPrice->event_flags = data_.eventFlags;
    graalTheoPrice->index = data_.index;
    graalTheoPrice->price = data_.price;
    graalTheoPrice->underlying_price = data_.underlyingPrice;
    graalTheoPrice->delta = data_.delta;
    graalTheoPrice->gamma = data_.gamma;
    graalTheoPrice->dividend = data_.dividend;
    graalTheoPrice->interest = data_.interest;
}

std::shared_ptr<TheoPrice> TheoPrice::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TheoPrice. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_THEO_PRICE) {
        throw InvalidArgumentException(fmt::format("Unable to create TheoPrice. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_THEO_PRICE)));
    }

    auto theoPrice = std::make_shared<TheoPrice>();

    theoPrice->fillData(graalNative);

    return theoPrice;
}

void *TheoPrice::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTheoPrice = new dxfg_theo_price_t{};

    fillGraalData(graalTheoPrice);

    return graalTheoPrice;
}

void TheoPrice::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_THEO_PRICE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free TheoPrice's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_THEO_PRICE)));
    }

    const auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTheoPrice;
}

void TheoPrice::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<TheoPrice>(); other) {
        data_ = other->data_;
    }
}

TheoPrice::TheoPrice() noexcept {
}

TheoPrice::TheoPrice(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const IndexedEventSource &TheoPrice::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t TheoPrice::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask TheoPrice::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void TheoPrice::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

void TheoPrice::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

std::int64_t TheoPrice::getIndex() const noexcept {
    return data_.index;
}

void TheoPrice::setIndex(std::int64_t index) {
    data_.index = index;
}

std::int64_t TheoPrice::getTime() const noexcept {
    return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void TheoPrice::setTime(std::int64_t time) noexcept {
    data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                            sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                       getSequence());
}

std::int32_t TheoPrice::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
}

void TheoPrice::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

double TheoPrice::getPrice() const noexcept {
    return data_.price;
}

void TheoPrice::setPrice(double price) noexcept {
    data_.price = price;
}

double TheoPrice::getUnderlyingPrice() const noexcept {
    return data_.underlyingPrice;
}

void TheoPrice::setUnderlyingPrice(double underlyingPrice) noexcept {
    data_.underlyingPrice = underlyingPrice;
}

double TheoPrice::getDelta() const noexcept {
    return data_.delta;
}

void TheoPrice::setDelta(double delta) noexcept {
    data_.delta = delta;
}

double TheoPrice::getGamma() const noexcept {
    return data_.gamma;
}

void TheoPrice::setGamma(double gamma) noexcept {
    data_.gamma = gamma;
}

double TheoPrice::getDividend() const noexcept {
    return data_.dividend;
}

void TheoPrice::setDividend(double dividend) noexcept {
    data_.dividend = dividend;
}

double TheoPrice::getInterest() const noexcept {
    return data_.interest;
}

void TheoPrice::setInterest(double interest) noexcept {
    data_.interest = interest;
}

std::string TheoPrice::toString() const {
    return fmt::format(
        "TheoPrice{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, underlyingPrice={}, "
        "delta={}, gamma={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getUnderlyingPrice()), dxfcpp::toString(getDelta()),
        dxfcpp::toString(getGamma()), dxfcpp::toString(getDividend()), dxfcpp::toString(getInterest()));
}

DXFCPP_END_NAMESPACE