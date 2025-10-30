// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/event/market/Order.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Order::TYPE = EventTypeEnum::ORDER;

void Order::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    orderData_ = {.marketMaker = toStringOpt(graalOrder->market_maker)};
}

void Order::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillGraalData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    graalOrder->order_base.market_event.event_type.clazz = DXFG_EVENT_ORDER;
    graalOrder->market_maker = createCString(orderData_.marketMaker);
}

void Order::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    delete[] graalOrder->market_maker;
}

std::shared_ptr<Order> Order::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Order. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ORDER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create Order. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_ORDER)));
    }

    auto order = std::make_shared<Order>();

    order->fillData(graalNative);

    return order;
}

void *Order::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOrder = new dxfg_order_t{};

    fillGraalData(graalOrder);

    return graalOrder;
}

void Order::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ORDER) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Order's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_ORDER)));
    }

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOrder;
}

void Order::assign(std::shared_ptr<EventType> event) {
    OrderBase::assign(event);

    if (const auto other = event->sharedAs<Order>(); other) {
        orderData_ = other->orderData_;
    }
}

Order::Order(const StringLike &eventSymbol) noexcept : OrderBase(eventSymbol) {
}

Order &Order::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

Order &Order::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

Order &Order::withSource(const OrderSource &source) noexcept {
    setSource(source);

    return *this;
}

Order &Order::withEventFlags(std::int32_t eventFlags) noexcept {
    OrderBase::setEventFlags(eventFlags);

    return *this;
}

Order &Order::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    OrderBase::setEventFlags(eventFlags);

    return *this;
}

Order &Order::withIndex(std::int64_t index) noexcept {
    OrderBase::setIndex(index);

    return *this;
}

Order &Order::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

Order &Order::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    setTimeNanoPart(timeNanoPart);

    return *this;
}

Order &Order::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

Order &Order::withTimeNanos(std::int64_t timeNanos) noexcept {
    setTimeNanos(timeNanos);

    return *this;
}

Order &Order::withAction(const OrderAction &action) noexcept {
    setAction(action);

    return *this;
}

Order &Order::withActionTime(std::int64_t actionTime) noexcept {
    setActionTime(actionTime);

    return *this;
}

Order &Order::withOrderId(std::int64_t orderId) noexcept {
    setOrderId(orderId);

    return *this;
}

Order &Order::withAuxOrderId(std::int64_t auxOrderId) noexcept {
    setAuxOrderId(auxOrderId);

    return *this;
}

Order &Order::withPrice(double price) noexcept {
    setPrice(price);

    return *this;
}

Order &Order::withSize(double size) noexcept {
    setSize(size);

    return *this;
}

Order &Order::withExecutedSize(double executedSize) noexcept {
    setExecutedSize(executedSize);

    return *this;
}

Order &Order::withCount(std::int64_t count) noexcept {
    setCount(count);

    return *this;
}

Order &Order::withTradeId(std::int64_t tradeId) noexcept {
    setTradeId(tradeId);

    return *this;
}

Order &Order::withTradePrice(double tradePrice) noexcept {
    setTradePrice(tradePrice);

    return *this;
}

Order &Order::withTradeSize(double tradeSize) noexcept {
    setTradeSize(tradeSize);

    return *this;
}

Order &Order::withExchangeCode(char exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

Order &Order::withExchangeCode(std::int16_t exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

Order &Order::withOrderSide(const Side &side) noexcept {
    setOrderSide(side);

    return *this;
}

Order &Order::withScope(const Scope &scope) noexcept {
    setScope(scope);

    return *this;
}

const std::string &Order::getMarketMaker() const & noexcept {
    if (!orderData_.marketMaker) {
        return String::NUL;
    }

    return orderData_.marketMaker.value();
}

const std::optional<std::string> &Order::getMarketMakerOpt() const & noexcept {
    return orderData_.marketMaker;
}

void Order::setMarketMaker(const StringLike &marketMaker) noexcept {
    orderData_.marketMaker = std::string(marketMaker);
}

Order &Order::withMarketMaker(const StringLike &marketMaker) noexcept {
    setMarketMaker(marketMaker);

    return *this;
}

std::string Order::toString() const {
    return fmt::format("Order{{{}, marketMaker={}}}", baseFieldsToString(), getMarketMaker());
}

DXFCPP_END_NAMESPACE