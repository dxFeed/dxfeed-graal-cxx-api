// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OrderImbalance.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../internal/utils/JavaUtils.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &OrderImbalance::TYPE = EventTypeEnum::ORDER_IMBALANCE;

void OrderImbalance::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalOrderImbalance = static_cast<dxfg_order_imbalance_t *>(graalNative);

    data_ = {
        .sourceId = graalOrderImbalance->source_id,
        .timeSequence = graalOrderImbalance->time_sequence,
        .refPrice = graalOrderImbalance->ref_price,
        .pairedSize = graalOrderImbalance->paired_size,
        .imbalanceSize = graalOrderImbalance->imbalance_size,
        .nearPrice = graalOrderImbalance->near_price,
        .farPrice = graalOrderImbalance->far_price,
        .flags = graalOrderImbalance->flags,
    };
}

void OrderImbalance::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalOrderImbalance = static_cast<dxfg_order_imbalance_t *>(graalNative);

    graalOrderImbalance->market_event.event_type.clazz = DXFG_EVENT_ORDER_IMBALANCE;
    graalOrderImbalance->source_id = data_.sourceId;
    graalOrderImbalance->time_sequence = data_.timeSequence;
    graalOrderImbalance->ref_price = data_.refPrice;
    graalOrderImbalance->paired_size = data_.pairedSize;
    graalOrderImbalance->imbalance_size = data_.imbalanceSize;
    graalOrderImbalance->near_price = data_.nearPrice;
    graalOrderImbalance->far_price = data_.farPrice;
    graalOrderImbalance->flags = data_.flags;
}

void OrderImbalance::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);
}

std::shared_ptr<OrderImbalance> OrderImbalance::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create OrderImbalance. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ORDER_IMBALANCE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create OrderImbalance. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_ORDER_IMBALANCE)));
    }

    auto orderImbalance = std::make_shared<OrderImbalance>();

    orderImbalance->fillData(graalNative);

    return orderImbalance;
}

void *OrderImbalance::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOrderImbalance = new dxfg_order_imbalance_t{};

    fillGraalData(graalOrderImbalance);

    return graalOrderImbalance;
}

void OrderImbalance::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ORDER_IMBALANCE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free OrderImbalance's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_ORDER_IMBALANCE)));
    }

    const auto graalOrderImbalance = static_cast<dxfg_order_imbalance_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOrderImbalance;
}

void OrderImbalance::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<OrderImbalance>(); other) {
        data_ = other->data_;
    }
}

OrderImbalance::OrderImbalance() noexcept {
}

OrderImbalance::OrderImbalance(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

OrderImbalance &OrderImbalance::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

OrderImbalance &OrderImbalance::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

const OrderSource &OrderImbalance::getSource() const & {
    return OrderSource::valueOf(data_.sourceId);
}

void OrderImbalance::setSource(const OrderSource &source) noexcept {
    data_.sourceId = source.id();
}

OrderImbalance &OrderImbalance::withSource(const OrderSource &source) noexcept {
    setSource(source);

    return *this;
}

std::int64_t OrderImbalance::getTimeSequence() const noexcept {
    return data_.timeSequence;
}

void OrderImbalance::setTimeSequence(std::int64_t timeSequence) noexcept {
    data_.timeSequence = timeSequence;
}

OrderImbalance &OrderImbalance::withTimeSequence(std::int64_t timeSequence) noexcept {
    setTimeSequence(timeSequence);

    return *this;
}

std::int64_t OrderImbalance::getTime() const noexcept {
    return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
           andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void OrderImbalance::setTime(std::int64_t time) noexcept {
    data_.timeSequence =
        orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                  sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
             getSequence());
}

OrderImbalance &OrderImbalance::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

std::int32_t OrderImbalance::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.timeSequence, MAX_SEQUENCE));
}

void OrderImbalance::setSequence(std::int32_t sequence) {
    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    data_.timeSequence = orOp(andOp(data_.timeSequence, ~static_cast<std::int64_t>(MAX_SEQUENCE)), sequence);
}

OrderImbalance &OrderImbalance::withSequence(std::int32_t sequence) {
    setSequence(sequence);

    return *this;
}

double OrderImbalance::getRefPrice() const noexcept {
    return data_.refPrice;
}

void OrderImbalance::setRefPrice(double refPrice) noexcept {
    data_.refPrice = refPrice;
}

OrderImbalance &OrderImbalance::withRefPrice(double refPrice) noexcept {
    setRefPrice(refPrice);

    return *this;
}

double OrderImbalance::getPairedSize() const noexcept {
    return data_.pairedSize;
}

void OrderImbalance::setPairedSize(double pairedSize) noexcept {
    data_.pairedSize = pairedSize;
}

OrderImbalance &OrderImbalance::withPairedSize(double pairedSize) noexcept {
    setPairedSize(pairedSize);

    return *this;
}

double OrderImbalance::getImbalanceSize() const noexcept {
    return data_.imbalanceSize;
}

void OrderImbalance::setImbalanceSize(double imbalanceSize) noexcept {
    data_.imbalanceSize = imbalanceSize;
}

OrderImbalance &OrderImbalance::withImbalanceSize(double imbalanceSize) noexcept {
    setImbalanceSize(imbalanceSize);

    return *this;
}

double OrderImbalance::getNearPrice() const noexcept {
    return data_.nearPrice;
}

void OrderImbalance::setNearPrice(double nearPrice) noexcept {
    data_.nearPrice = nearPrice;
}

OrderImbalance &OrderImbalance::withNearPrice(double nearPrice) noexcept {
    setNearPrice(nearPrice);

    return *this;
}

double OrderImbalance::getFarPrice() const noexcept {
    return data_.farPrice;
}

void OrderImbalance::setFarPrice(double farPrice) noexcept {
    data_.farPrice = farPrice;
}

OrderImbalance &OrderImbalance::withFarPrice(double farPrice) noexcept {
    setFarPrice(farPrice);

    return *this;
}

const ImbalanceSide &OrderImbalance::getImbalanceSide() const & noexcept {
    return ImbalanceSide::valueOf(getBits(data_.flags, IMBALANCE_SIDE_MASK, IMBALANCE_SIDE_SHIFT));
}

void OrderImbalance::setImbalanceSide(const ImbalanceSide &side) noexcept {
    data_.flags = setBits(data_.flags, IMBALANCE_SIDE_MASK, IMBALANCE_SIDE_SHIFT, side.getCode());
}

OrderImbalance &OrderImbalance::withImbalanceSide(const ImbalanceSide &side) noexcept {
    setImbalanceSide(side);

    return *this;
}

const AuctionType &OrderImbalance::getAuctionType() const & noexcept {
    return AuctionType::valueOf(getBits(data_.flags, AUCTION_TYPE_MASK, AUCTION_TYPE_SHIFT));
}

void OrderImbalance::setAuctionType(const AuctionType &type) noexcept {
    data_.flags = setBits(data_.flags, AUCTION_TYPE_MASK, AUCTION_TYPE_SHIFT, type.getCode());
}

OrderImbalance &OrderImbalance::withAuctionType(const AuctionType &type) noexcept {
    setAuctionType(type);

    return *this;
}

std::string OrderImbalance::toString() const {
    return fmt::format("OrderImbalance{{{}, eventTime={}, source={}, time={}, sequence={}, refPrice={}, "
                       "pairedSize={}, imbalanceSize={}, nearPrice={}, farPrice={}, imbalanceSide={}, "
                       "auctionType={}}}",
                       MarketEvent::getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()), getSource().toString(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
                       java::doubleToString(getRefPrice()), java::doubleToString(getPairedSize()),
                       java::doubleToString(getImbalanceSize()), java::doubleToString(getNearPrice()),
                       java::doubleToString(getFarPrice()), getImbalanceSide().toString(), getAuctionType().toString());
}

DXFCPP_END_NAMESPACE
