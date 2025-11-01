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

std::string TheoPrice::toString() const {
    return fmt::format(
        "TheoPrice{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, underlyingPrice={}, "
        "delta={}, gamma={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getUnderlyingPrice()), dxfcpp::toString(getDelta()),
        dxfcpp::toString(getGamma()), dxfcpp::toString(getDividend()), dxfcpp::toString(getInterest()));
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

void TheoPrice::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

DXFCPP_END_NAMESPACE